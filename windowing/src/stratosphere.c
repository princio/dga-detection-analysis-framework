
#include "stratosphere.h"

#include "calculator.h"
#include "parameters.h"
#include "persister.h"
#include "windowing.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char GALAXY_NAME[] = "stratosphere";

WindowingGalaxy* GALAXY_PTR;

int32_t PREPARED = 0;

static PGconn *conn = NULL;

void disconnect() {
    PQfinish(conn);
    conn = NULL;
}

int connect() {
    if (conn != NULL) {
        return 0;
    }

    conn = PQconnectdb("postgresql://princio:postgres@localhost/dns");

    if (PQstatus(conn) == CONNECTION_OK) {
        return 0;
    }
    
    fprintf(stderr, "CONNECTION_BAD %s\n", PQerrorMessage(conn));

    disconnect();

    return -1;
}

void parse_message(PGresult* res, int row, DNSMessage* message) {
    message->fn_req = atoi(PQgetvalue(res, row, 0));
    message->value = atof(PQgetvalue(res, row, 1));
    message->logit = atof(PQgetvalue(res, row, 2));
    message->is_response = PQgetvalue(res, row, 3)[0] == 't';
    message->top10m = atoi(PQgetvalue(res, row, 4));
    message->dyndns = PQgetvalue(res, row, 5)[0] == 't';
    message->id = atol(PQgetvalue(res, row, 6));
}

void printdatastring(PGresult* res) {
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data\n");
    }
    else {
        int ncols = PQnfields(res);
        int nrows = 5;

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

int get_pcaps_number() {
    if (!connect()) {
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

int32_t get_fnreq_max(int32_t id) {
    int fnreq_max;
    char sql[1000];
    sprintf(sql, "SELECT MAX(FN_REQ) FROM MESSAGES_%d", id);

    PGresult* res_nrows = PQexec(conn, sql);
    if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Get MAX(FN_REQ) for source %d failed\n", __FILE__, __LINE__, id);
    } else {
        fnreq_max = atoi(PQgetvalue(res_nrows, 0, 0));
    }
    PQclear(res_nrows);

    return fnreq_max;
}


void fetch_sources_and_add_to_experiment() {
    PGresult* pgresult;

    pgresult = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r, fnreq_max FROM pcap JOIN malware as mw ON malware_id = mw.id ORDER BY qr ASC");

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Get pcaps failed: %s\n", __FILE__, __LINE__, PQerrorMessage(conn));
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    for(int r = 0; r < nrows; r++) {
        Source* source;
        char name[10];
    
        {
            int z = 0;
            source->id = atoi(PQgetvalue(pgresult, r, z++));
            source->dgaclass = atoi(PQgetvalue(pgresult, r, z++));
            source->qr = atoi(PQgetvalue(pgresult, r, z++));
            source->q = atoi(PQgetvalue(pgresult, r, z++));
            source->r = atoi(PQgetvalue(pgresult, r, z++));
            source->fnreq_max = atoi(PQgetvalue(pgresult, r, z++));
        }

        source->capture_type = CAPTURETYPE_PCAP;
        sprintf(source->name, "%s_%d", GALAXY_NAME, source->id);

        windowing_sources_add(GALAXY_PTR, source, perform_windowing);
    }

    PQclear(pgresult);
}

void fetch_source_messages(const Source* source, int32_t* nrows, PGresult** pgresult) {
    char sql[1000];
    int pgresult_binary = 1;

    printf("id: %d\n", source->id);
    
    sprintf(sql,
            "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS, M.ID FROM MESSAGES_%d AS M "
            "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            "JOIN DN AS DN ON M.DN_ID=DN.ID "
            "ORDER BY FN_REQ, ID",
            source->id
            );

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Select messages error for pcap %d: %s\n", __FILE__, __LINE__, source->id, PQerrorMessage(conn));
        PQclear(pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);

    if ((*nrows) != source->qr) {
        printf("[%s:%d] nrows != source->qr:\t%d != %ld\n", __FILE__, __LINE__, *nrows, source->qr);

        if ((*nrows) > source->qr) {
            (*nrows) = source->qr; // we may occur in a new unallocated window
        }
    }
}

void perform_windowing(const Source* source, MANY(WindowingWindows) wws, const int32_t row) {
    PGresult* pgresult;
    int nrows;

    fetch_source_messages(&source, &nrows, &pgresult);

    for(int r = 0; r < nrows; r++) {
        DNSMessage message;
        WindowingWindows* const sw_row = &wws._[row];

        parse_message(pgresult, r, &message);

        windowing_message(message, windowing.psets, sw_row);
    }
}

/*
MANY(Dataset) stratosphere_bo() {
    Windowings windowings;
    
    INITMANY(datasets, experiment.psets.number, Dataset);
    INITMANY(windowings, GALAXY_PTR->sources->number, Windowing);

    for (int32_t d = 0; d < datasets.number; d++) {
        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
            datasets._[d].windows[cl].number = GALAXY_PTR->nwindows[cl];
            datasets._[d].windows[cl]._ = calloc(n_windows[d][cl], sizeof(Window));
        }
    }

    Windowings* windowings[N_DGACLASSES];
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        windowings[cl][GALAXY_PTR->sources[cl].number];

        int32_t nw = 0;
        for (int32_t s = 0; s < GALAXY_PTR->sources[cl].number; s++) {
            windowings[s] = stratosphere_source_perform(GALAXY_PTR->sources[cl]._[s]);
        }

        for (int32_t d = 0; d < psets.number; d++) {
            datasets._[d]->windows[cl].number = n_windows[d][cl];
            datasets._[d].windows[cl]._ = calloc(n_windows[d][cl], sizeof(Window));
            memcpy(&datasets._[d].windows[cl]._, windowings[s]., windoweds[s][d].number * sizeof(Window));
        }
    }

    // for (int32_t d = 0; d < psets.number; d++) {
    //     Dataset* dataset = &datasets._[d];

    //     for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
    //         dataset->windows[cl].number = n_windows[d][cl];
    //         dataset->windows[cl]._ = calloc(n_windows[d][cl], sizeof(Window));
    //     }
    // }

    // int32_t n_cursor[psets.number][N_DGACLASSES];
    // memset(n_cursor, 0, sizeof(int32_t) * N_DGACLASSES * datasets.number);
    // for (int32_t s = 0; s < sources->number; s++) {
    //     if (windoweds_loaded[s] == datasets.number) {
    //         // printf("Source %d loaded from disk\n", s);
    //     } else {
    //         MANY(Window__s) windowss = stratosphere_source_perform(&sources->_[s], psets);
    //     }

    //     DGAClass class = sources->_[s].dgaclass;

    //     for (int32_t d = 0; d < datasets.number; d++) {
    //         if (n_cursor[d][class] + windoweds[s][d].number > datasets._[d].windows[class].number) {
    //             printf("[%s:%d] Error: (%d + %d) > %d\n", __FILE__, __LINE__, n_cursor[d][class], windoweds[s][d].number, datasets._[d].windows[class].number);
    //         }

    //         memcpy(&datasets._[d].windows[class]._[n_cursor[d][class]], windoweds[s][d]._, windoweds[s][d].number * sizeof(Window));
    //         n_cursor[d][class] += windoweds[s][d].number;

    //         free(windoweds[s][d]._);
    //     }
    // }
}
*/

void stratosphere_prepare() {
    if (!PREPARED) {
        if (connect()) {
            printf("Stratosphere: cannot connect to database.");
            return;
        }

        GALAXY_PTR = windowing_galaxy_add("stratosphere");

        fetch_sources_and_add_to_experiment();

        disconnect();

        PREPARED = 1;
    }
}