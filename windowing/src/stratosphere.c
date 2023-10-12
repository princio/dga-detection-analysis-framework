
#include "stratosphere.h"

#include "calculator.h"
#include "persister.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static PGconn *conn = NULL;

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


Sources stratosphere_get_sources() {
    PGresult* pgresult;
    Sources sources;


    if (!_connect()) {
        puts("Error!");
        exit(1);
    }

    // int number = _get_pcaps_number(conn);

    pgresult = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r FROM pcap JOIN malware as mw ON malware_id = mw.id ORDER BY qr ASC");

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Get pcaps failed: %s\n", __FILE__, __LINE__, PQerrorMessage(conn));
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    sources.number = nrows;
    sources._ = calloc(nrows, sizeof(Source));

    for(int source_idx = 0; source_idx < nrows; source_idx++) {
        Source* source = &sources._[source_idx];

        source->id = source_idx;

        source->galaxy_id = atoi(PQgetvalue(pgresult, source_idx, 0));

        source->capture_type = CAPTURETYPE_PCAP;

        source->class = atoi(PQgetvalue(pgresult, source_idx, 1));
        source->qr = atoi(PQgetvalue(pgresult, source_idx, 2));
        source->q = atoi(PQgetvalue(pgresult, source_idx, 3));
        source->r = atoi(PQgetvalue(pgresult, source_idx, 4));

        {
            int nmessages = 0;
            char sql[1000];
            // sprintf(sql, "SELECT COUNT(*) FROM MESSAGES_%ld AS M JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID JOIN DN AS DN ON M.DN_ID=DN.ID", capture[source_idx].id);
            sprintf(sql, "SELECT COUNT(*) FROM MESSAGES_%d", source->galaxy_id);

            PGresult* res_nrows = PQexec(conn, sql);
            if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                printf("[%s:%d] Count messages for source %d failed\n", __FILE__, __LINE__, source->galaxy_id);
            } else {
                nmessages = atoi(PQgetvalue(res_nrows, 0, 0));
            }
            PQclear(res_nrows);
            sources._[source_idx].nmessages = nmessages;
        }

        {
            int fnreq_max = 0;
            char sql[1000];
            // sprintf(sql, "SELECT COUNT(*) FROM MESSAGES_%ld AS M JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID JOIN DN AS DN ON M.DN_ID=DN.ID", capture[source_idx].id);
            sprintf(sql, "SELECT MAX(FN_REQ) FROM MESSAGES_%d", source->galaxy_id);

            PGresult* res_nrows = PQexec(conn, sql);
            if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                printf("[%s:%d] Get MAX(FN_REQ) for source %d failed\n", __FILE__, __LINE__, source->galaxy_id);
            } else {
                fnreq_max = atoi(PQgetvalue(res_nrows, 0, 0));
            }
            PQclear(res_nrows);
            sources._[source_idx].fnreq_max = fnreq_max;
        }
    }

    PQclear(pgresult);

    return sources;
}


void stratosphere_source_perform(Source* source, Dataset0s datasets, Windows* windows) {
    PGresult* pgresult;
    int nrows;

    if (!_connect()) {
        printf("[%s:%d] Connection error: %s\n", __FILE__, __LINE__, PQerrorMessage(conn));
        exit(1);
    }

    {
        char sql[1000];
        int pgresult_binary = 1;

        printf("id: %d\n", source->galaxy_id);
        
        sprintf(sql,
                "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS, M.ID FROM MESSAGES_%d AS M "
                "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                "JOIN DN AS DN ON M.DN_ID=DN.ID "
                "ORDER BY FN_REQ, ID",
                source->galaxy_id
                );

        pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

        if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
            printf("[%s:%d] Select messages error for pcap %d: %s\n", __FILE__, __LINE__, source->galaxy_id, PQerrorMessage(conn));
            PQclear(pgresult);
            return;
        }

        nrows = PQntuples(pgresult);

        if (nrows != source->nmessages) {
            printf("[%s:%d] nrows != source->nmessages:\t%d != %ld\n", __FILE__, __LINE__, nrows, source->nmessages);

            if (nrows > source->nmessages) {
                nrows = source->nmessages; // we may occur in a new unallocated window
            }
        }
    }

    // int wnum_max[N_WSIZE];
    // memset(wnum_max, 0, sizeof(int) * N_WSIZE);
    for(int r = 0; r < nrows; r++) {
        Message message;

        _parse_message(pgresult, r, &message);

        for (int widx = 0; widx < datasets.number; ++widx) {
            int wnum;
            Windowing* windowing = &datasets._[widx].windowing;
            Window *window;
            int32_t wsize;
            int32_t n_windows;

            wsize = windowing->wsize.value;
            n_windows = windows[widx].number;

            wnum = (int) floor(message.fn_req / wsize);

            if (wnum >= n_windows) {
                printf("ERROR\n");
                printf("      wnum: %d\n", wnum);
                printf("     wsize: %d\n", wsize);
                printf(" fnreq_max: %ld\n", source->fnreq_max);
                printf("  nwindows: %d\n", n_windows);
            }


            // wnum_max[ws] = wnum_max[ws] < wnum ? wnum : wnum_max[ws];

            window = &windows[widx]._[wnum];

            if (wnum != 0 && wnum != window->window_id) {
                printf("Error[wnum != window->wnum]:\t%d\t%d\n", wnum, window->window_id);
            }

            calculator_message(&message, window, windowing->pset);

        }
    }

    PQclear(pgresult);
}

int _persister__sources(PersisterReadWrite read, Experiment* exp, Sources* sources) {
    char subname[20];
    sprintf(subname, "stratosphere");
    return persister__sources(read, exp, subname, sources);
}

int _persister__windows(PersisterReadWrite read, Experiment* exp, int32_t source_id, int32_t dataset_id, Windows* windows) {
    char subname[20];
    sprintf(subname, "stratosphere_%d_%d", source_id, dataset_id);
    return persister__windows(read, exp, subname, windows);
}


void stratosphere_run(Experiment* exp, Dataset0s datasets, Sources* sources_ptr) {
    Sources sources;

    if (_persister__sources(PERSITER_READ, exp, &sources)) {
        sources = stratosphere_get_sources();
        _persister__sources(PERSITER_WRITE, exp, &sources);
    }

    Windows windoweds[sources.number][datasets.number];
    memset(windoweds, 0, sizeof(Windows) * sources.number * datasets.number);

    int32_t windoweds_loaded[sources.number];
    memset(windoweds_loaded, 0, sizeof(int32_t) * sources.number);

    int32_t n_windows[datasets.number][N_CLASSES];
    memset(n_windows, 0, sizeof(int32_t) * N_CLASSES * datasets.number);
    for (int32_t s = 0; s < sources.number; s++) {
        for (int32_t d = 0; d < datasets.number; d++) {

            if (0 == _persister__windows(PERSITER_READ, exp, s, d, &windoweds[s][d])) {
                // printf("Windows of Source %d (class %d) with Dataset %d loaded from disk\n", s, sources._[s].class, d);
                windoweds_loaded[s]++;

                n_windows[d][sources._[s].class] += windoweds[s][d].number;

                continue;
            }
    
            printf("Windows of Source %d with Dataset\n", s);

            int32_t nw = N_WINDOWS(sources._[s].fnreq_max, datasets._[d].windowing.wsize.value);
            windoweds[s][d].number = nw;
            windoweds[s][d]._ = calloc(windoweds[s][d].number, sizeof(Window));

            for (int32_t w = 0; w < windoweds[s][d].number; w++) {
                windoweds[s][d]._[w].source_id = sources._[s].id;
                windoweds[s][d]._[w].window_id = w;
                windoweds[s][d]._[w].dataset_id = d;
            }

            n_windows[d][sources._[s].class] += nw;
        }
    }

    for (int32_t d = 0; d < datasets.number; d++) {
        Dataset0* dataset = &datasets._[d];

        for (int32_t cl = 0; cl < N_CLASSES; cl++) {
            dataset->windows[cl].number = n_windows[d][cl];
            dataset->windows[cl]._ = calloc(n_windows[d][cl], sizeof(Window));
        }
    }

    int32_t n_cursor[datasets.number][N_CLASSES];
    memset(n_cursor, 0, sizeof(int32_t) * N_CLASSES * datasets.number);
    for (int32_t s = 0; s < sources.number; s++) {
        if (windoweds_loaded[s] == datasets.number) {
            // printf("Source %d loaded from disk\n", s);
        } else {
            stratosphere_source_perform(&sources._[s], datasets, windoweds[s]);

            for (int32_t d = 0; d < datasets.number; d++) {
                _persister__windows(PERSITER_WRITE, exp, s, d, &windoweds[s][d]);
            }
        }

        Class class = sources._[s].class;

        for (int32_t d = 0; d < datasets.number; d++) {
            if (n_cursor[d][class] + windoweds[s][d].number > datasets._[d].windows[class].number) {
                printf("[%s:%d] Error: (%d + %d) > %d\n", __FILE__, __LINE__, n_cursor[d][class], windoweds[s][d].number, datasets._[d].windows[class].number);
            }

            memcpy(&datasets._[d].windows[class]._[n_cursor[d][class]], windoweds[s][d]._, windoweds[s][d].number * sizeof(Window));
            n_cursor[d][class] += windoweds[s][d].number;


            free(windoweds[s][d]._);
        }
    }

    *sources_ptr = sources;
}

void stratoshere_disconnect() {
    PQfinish(conn);
    conn = NULL;
}
