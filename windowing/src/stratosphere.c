
#include "calculator.h"
#include "stratosphere.h"
#include "tester.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static PGconn *conn = NULL;

static FILE* ff;
static int _capture_index;
static int tries = 3;

void _disconnect(PGconn* conn) {
    PQfinish(conn);
}



void _parse_message(PGresult* res, int row, Message* message) {
    message->fn_req = atoi(PQgetvalue(res, row, 0));
    message->value = atof(PQgetvalue(res, row, 1));
    message->logit = atof(PQgetvalue(res, row, 2));
    message->is_response = PQgetvalue(res, row, 3)[0] == 't';
    message->top10m = atoi(PQgetvalue(res, row, 4));
    message->dyndns = PQgetvalue(res, row, 5)[0] == 't';
    message->id = atol(PQgetvalue(res, row, 6));


    // fprintf(ff, "%ld,%d,%s,", message->id, tries, PQgetvalue(res, row, 2));
    // for (int i = 0; i < 8; ++i) {
	// 	fprintf(ff, "%02X", ((unsigned char*)&message->logit)[i]);
	// }
    // fprintf(ff, "\n");
}



void printdatastring(PGresult* res) {
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data\n");
    }
    else {
        int ncols = PQnfields(res);
        int nrows = 5;//PQntuples(res);

        for(int r = 0; r < nrows; r++)
        {
            for (int c = 0; c < ncols; c++)
            {
                char* colname = PQfname(res, c);
                char* value = PQgetvalue(res, r, c);
                printf("%10s\t", colname);
                printf("%25s\t", value);

                Oid oid = PQftype(res, c);
                switch (PQftype(res, c)) {
                    case INT2OID:
                        printf("%20s\n", "INT2OID");
                    break;
                    case INT4OID:
                        printf("%20s\n", "INT4OID");
                    break;
                    case INT8OID: {
                        int64_t iptr = atol(value);
                        printf("%20ld\t", iptr);
                        printf("%20s[%zu]\n", "INT8OID", sizeof (int64_t));
                        break;
                    }
                    case FLOAT4OID: {
                        double f4ptr = atoi(value);
                        printf("%20f\t", f4ptr);
                        printf("%20s\n", "FLOAT4OID");
                        break;
                    }
                    case FLOAT8OID: {
                        double f8ptr = atof(value);
                        printf("%20f\t", f8ptr);
                        printf("%20s[%zu]\n", "FLOAT8OID", sizeof (double));
                        break;
                    }
                    case BOOLOID: {
                        int _bool = value[0] == 't';
                        printf("%20i\t", _bool);
                        printf("%20s\n", "BOOLOID");
                        break;
                    }
                    default:
                        printf("%20u\n", oid);
                }
            }

            puts("");
        }
    }
}


int _connect() {
    if (conn != NULL) {
        return 1;
    }

    conn = PQconnectdb("postgresql://princio:postgres@localhost/dns");

    if (PQstatus(conn) == CONNECTION_OK) {
        return 1;
    }
    
    // fprintf(stderr, "CONNECTION_BAD %s\n", PQerrorMessage(conn));

    _disconnect(conn);

    conn = NULL;

    return 0;
}


int _get_pcaps_number() {
    if (!_connect()) {
        puts("Error!");
        exit(1);
    }

    PGresult* res = PQexec(conn, "SELECT count(*) FROM pcap");
    int64_t rownumber = -1;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data\n");
    }
    else {
        char* srownumber = PQgetvalue(res, 0, 0);

        rownumber = atoi(srownumber);
    }

    return rownumber;
}



void stratosphere_procedure(WindowingPtr windowing, int32_t capture_index) {
    ff = fopen("/tmp/bib.csv", "a");
    _capture_index = capture_index;

    PGresult* pgresult;
    int nrows;
    const int N_WSIZE = windowing->wsizes.number;
    Capture* capture = &windowing->captures._[capture_index];
    WSet* capture_wsets = windowing->captures_wsets[capture_index];

    if (!_connect()) {
        puts("Error!");
        exit(1);
    }

    {
        char sql[1000];
        int pgresult_binary = 1;

        sprintf(sql,
                "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS, M.ID FROM MESSAGES_%d AS M "
                "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                "JOIN DN AS DN ON M.DN_ID=DN.ID "
                "ORDER BY FN_REQ, ID",
                capture->id);

        pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

        if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
            printf("No data\n");
            PQclear(pgresult);
            return;
        }

        nrows = PQntuples(pgresult);

        if (nrows != capture->nmessages) {
            printf("Error[nrows != capture->nmessages]:\t%d\t%ld\n", nrows, capture->nmessages);
        }
    }

    int wnum_max[N_WSIZE];
    memset(wnum_max, 0, sizeof(int) * N_WSIZE);
    for(int r = 0; r < nrows; r++) {
        Message message;

        _parse_message(pgresult, r, &message);

        for (int ws = 0; ws < N_WSIZE; ++ws) {
            int wnum;
            Window *window;
            int32_t wsize;
            int32_t n_windows;

            wsize = windowing->wsizes._[ws];
            n_windows = capture_wsets[ws].number;

            wnum = (int) floor(message.fn_req / wsize);

            if (wnum >= n_windows) {
                printf("ERROR\n");
                printf("      wnum: %d\n", wnum);
                printf("     wsize: %d\n", wsize);
                printf(" fnreq_max: %ld\n", capture->fnreq_max);
                printf("  nwindows: %d\n", n_windows);
            }


            wnum_max[ws] = wnum_max[ws] < wnum ? wnum : wnum_max[ws];

            window = &capture_wsets[ws]._[wnum];

            if (wnum != window->wnum) {
                printf("Error[wnum != window->wnum]:\t%d\t%d\n", wnum, window->wnum);
            }

            calculator_message(&message, &window->metrics, windowing->psets._);

            if (ws == 0) {
                const int S = window->metrics.number * sizeof(WindowMetricSet);
                tester_stratosphere(trys, capture_index, wnum, ws, message.id, r, window->metrics);
            }
        }
    }


    // for (int ws = 0; ws < windowing->wsizes.number; ws++) {
    //     WSet* wset = &windowing->captures_wsets[capture_index][ws];
    //     Window* windows = wset->_;
    //     for (int wn = 0; wn < wset->number; wn++) {
    //         const int S = windows[wn].metrics.number * sizeof(WindowMetricSet);
            
    //         uint8_t* a;
    //         FILE* ff = fopen("./ciao.csv", "a");
    //         fprintf(ff, "%s,%d,%d,%d,%d,", try_name, trys, capture_index, ws, wn);

    //         a = (uint8_t*) windows[wn].metrics._;
    //         for (int q = 0; q < S; ++q) {
    //             fprintf(ff, "%02X", a[q]);
    //         }
    //         fprintf(ff, "\n");
    //         fclose(ff);
    //     }
    // }
    
    // for (int w0 = 0; w0 < N_WSIZE; ++w0) {
    //     printf("wnum_max: %d / %d -- %ld / %ld\n", wnum_max[w0], windowing->wsizes._[w0], capture->fnreq_max, N_WINDOWS(capture->fnreq_max, windowing->wsizes._[w0]));
    // }


    PQclear(pgresult);

    fclose(ff);
}



void stratosphere_add_captures(WindowingPtr windowing) {
    Captures* captures = &windowing->captures;
    PGresult* pgresult;

    if (!_connect()) {
        puts("Error!");
        exit(1);
    }

    // int number = _get_pcaps_number(conn);

    pgresult = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r FROM pcap JOIN malware as mw ON malware_id = mw.id ORDER BY qr ASC");

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("No data\t%s\n", PQerrorMessage(conn));
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    int32_t capture_index = windowing->captures.number;

    for(int r = 0; r < nrows; r++) {
        captures->_[capture_index].id = atoi(PQgetvalue(pgresult, r, 0));

        captures->_[capture_index].capture_type = CAPTURETYPE_PCAP;

        captures->_[capture_index].class = atoi(PQgetvalue(pgresult, r, 1));
        captures->_[capture_index].qr = atoi(PQgetvalue(pgresult, r, 2));
        captures->_[capture_index].q = atoi(PQgetvalue(pgresult, r, 3));
        captures->_[capture_index].r = atoi(PQgetvalue(pgresult, r, 4));

        captures->_[capture_index].fetch = (FetchPtr) &stratosphere_procedure;

        {
            int nmessages = 0;
            char sql[1000];
            // sprintf(sql,
            //     "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
            //     "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            //     "JOIN DN AS DN ON M.DN_ID=DN.ID",
            //     capture[capture_index].id);
            sprintf(sql,
                "SELECT COUNT(*) FROM MESSAGES_%d",
                captures->_[capture_index].id);

            PGresult* res_nrows = PQexec(conn, sql);
            if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                printf("No data\n");
            } else {
                nmessages = atoi(PQgetvalue(res_nrows, 0, 0));
            }
            PQclear(res_nrows);
            captures->_[capture_index].nmessages = nmessages;
        }

        {
            int fnreq_max = 0;
            char sql[1000];
            // sprintf(sql,
            //     "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
            //     "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            //     "JOIN DN AS DN ON M.DN_ID=DN.ID",
            //     capture[capture_index].id);
            sprintf(sql,
                "SELECT MAX(FN_REQ) FROM MESSAGES_%d",
                captures->_[capture_index].id);

            PGresult* res_nrows = PQexec(conn, sql);
            if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                printf("No data\n");
            } else {
                fnreq_max = atoi(PQgetvalue(res_nrows, 0, 0));
            }
            PQclear(res_nrows);
            captures->_[capture_index].fnreq_max = fnreq_max;
        }

        ++capture_index;
    }

    windowing->captures.number = capture_index;

    PQclear(pgresult);
}



void stratoshere_disconnect() {
    PQfinish(conn);
    conn = NULL;
}
