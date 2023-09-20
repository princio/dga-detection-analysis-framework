
#include "stratosphere.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>
#include <stdint.h>


#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0


static PGconn *conn = NULL;



void _parse_message(PGresult* res, int row, Message* message) {
    message->fn_req = atoi(PQgetvalue(res, row, 0));
    message->value = atof(PQgetvalue(res, row, 1));
    message->logit = atof(PQgetvalue(res, row, 2));
    message->is_response = PQgetvalue(res, row, 3)[0] == 't';
    message->top10m = atoi(PQgetvalue(res, row, 4));
    message->dyndns = PQgetvalue(res, row, 5)[0] == 't';
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


PGconn* _get_connection() {
    if (conn != NULL) {
        return conn;
    }

    PGconn *conn = PQconnectdb("postgresql://princio:postgres@localhost/dns");

    if (PQstatus(conn) == CONNECTION_OK) {
        puts("CONNECTION_OK\n");

        return conn;
    }
    
    fprintf(stderr, "CONNECTION_BAD %s\n", PQerrorMessage(conn));

    CDPGclose_connection(conn);

    return NULL;
}


int _get_pcaps_number() {
    PGconn *_conn = _get_connection();
    if (_conn == NULL) {
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



void stratosphere_procedure(char* root_dir, WindowingPtr windowing, int32_t capture_index) {
    PGresult* pgresult;
    int ret;
    int nrows;
    const int N_WSIZE = windowing->n_wsizes;
    Capture* capture = &windowing->captures[capture_index];
    CaptureWindowingPtr capture_windowings = &windowing->captures_windowings[capture_index];

    ret = persister_read__capture_windowings(root_dir, windowing, capture_index);

    if (ret == 1) {
        return;
    }

    {
        char sql[1000];
        int pgresult_binary = 1;

        sprintf(sql,
                "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS FROM MESSAGES_%ld AS M "
                "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                "JOIN DN AS DN ON M.DN_ID=DN.ID "
                "ORDER BY FN_REQ",
                capture->id);

        pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);
    }

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("No data\n");
        return;
    }

    nrows = PQntuples(pgresult);

    if (nrows != capture->nmessages) {
        printf("Errorrrrrr:\t%d\t%ld\n", nrows, capture->nmessages);
    }

    { // allocating windows and metrics inside each capture_window
        for (int32_t i = 0; i < N_WSIZE; ++i) {
            int32_t wsize = windowing->wsizes[i];

            int32_t n_windows = N_WINDOWS(capture->fnreq_max, wsize);

            capture_windowings[i].n_windows = n_windows;

            capture_windowings[i].windows = calloc(n_windows, sizeof(Window));

            Window* windows = capture_windowings[i].windows;
            for (int32_t w = 0; w < n_windows; ++w) {

                windows[w].infected_type = capture->infected_type;
                windows[w].metrics = calloc(windowing->n_psets, sizeof(WindowMetrics));
            }
        }
    }

    int wnum_max[N_WSIZE];
    memset(wnum_max, 0, sizeof(int) * N_WSIZE);
    for(int r = 0; r < nrows; r++) {
        Message message;

        _parse_message(pgresult, r, &message);

        for (int w0 = 0; w0 < N_WSIZE; ++w0) {
            int wnum;
            Window *window;
            int32_t wsize;
            int32_t n_windows;

            wsize = windowing->wsizes[w0];
            n_windows = capture_windowings[w0].n_windows;

            wnum = (int) floor(message.fn_req / wsize);

            if (wnum >= n_windows) {
                printf("ERROR\n");
                printf("      wnum: %d\n", wnum);
                printf("     wsize: %d\n", wsize);
                printf(" fnreq_max: %ld\n", capture->fnreq_max);
                printf("  nwindows: %d\n", n_windows);
            }


            wnum_max[w0] = wnum_max[w0] < wnum ? wnum : wnum_max[w0];

            window = &capture_windowings[w0].windows[wnum];

            if (wnum != window->wnum) {
                printf("Errorrrrrr:\t%d\t%d\n", wnum, window->wnum);
            }

            calculator_message(&message, window->nmetrics, window->metrics);
        }
    }
    

    for (int w0 = 0; w0 < N_WSIZE; ++w0) {
        printf("wnum_max: %d / %d -- %ld / %ld\n", wnum_max[w0], windowing->wsizes[w0], capture->fnreq_max, N_WINDOWS(capture->fnreq_max, windowing->wsizes[w0]));
    }


    PQclear(pgresult);
}



void stratosphere_add_captures(char* root_dir, WindowingPtr windowing) {
    int ret;
    int n_captures;
    Captures captures;
    PGresult* pgresult;

    PGconn *_conn = _get_connection();
    if (_conn == NULL) {
        puts("Error!");
        exit(1);
    }

    n_captures = _get_pcaps_number(conn);

    pgresult = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r FROM pcap JOIN malware as mw ON malware_id = mw.id");

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("No data\n");
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    int32_t capture_index = windowing->n_captures;

    for(int r = 0; r < nrows; r++) {
        captures[capture_index].id = atoi(PQgetvalue(pgresult, r, 0));

        captures[capture_index].capture_type = CAPTURETYPE_PCAP;

        captures[capture_index].infected_type = atoi(PQgetvalue(pgresult, r, 1));
        captures[capture_index].qr = atoi(PQgetvalue(pgresult, r, 2));
        captures[capture_index].q = atoi(PQgetvalue(pgresult, r, 3));
        captures[capture_index].r = atoi(PQgetvalue(pgresult, r, 4));

        captures[capture_index].fetch = &stratosphere_procedure;

        {
            int nmessages = 0;
            char sql[1000];
            // sprintf(sql,
            //     "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
            //     "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            //     "JOIN DN AS DN ON M.DN_ID=DN.ID",
            //     captures[capture_index].id);
            sprintf(sql,
                "SELECT COUNT(*) FROM MESSAGES_%ld",
                captures[capture_index].id);

            PGresult* res_nrows = PQexec(conn, sql);
            if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                printf("No data\n");
            } else {
                nmessages = atoi(PQgetvalue(res_nrows, 0, 0));
            }
            PQclear(res_nrows);
            captures[capture_index].nmessages = nmessages;
        }

        {
            int fnreq_max = 0;
            char sql[1000];
            // sprintf(sql,
            //     "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
            //     "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            //     "JOIN DN AS DN ON M.DN_ID=DN.ID",
            //     captures[capture_index].id);
            sprintf(sql,
                "SELECT MAX(FN_REQ) FROM MESSAGES_%ld",
                captures[capture_index].id);

            PGresult* res_nrows = PQexec(conn, sql);
            if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                printf("No data\n");
            } else {
                fnreq_max = atoi(PQgetvalue(res_nrows, 0, 0));
            }
            PQclear(res_nrows);
            captures[capture_index].fnreq_max = fnreq_max;
        }

        ++capture_index;
    }

    PQclear(pgresult);
}



void stratoshere_disconnect() {
    PQfinish(conn);
    conn = NULL;
}
