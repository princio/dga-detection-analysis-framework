
#include "stratosphere.h"

#include "parameters.h"
#include "testbed2.h"
#include "window0s.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char GALAXY_NAME[] = "stratosphere";

int32_t PREPARED = 0;

static PGconn *conn = NULL;

void _stratosphere_disconnect() {
    PQfinish(conn);
    conn = NULL;
}

int _stratosphere_connect() {
    if (conn != NULL) {
        return 0;
    }
    conn = PQconnectdb("postgresql://princio:postgres@localhost/dns");

    if (PQstatus(conn) == CONNECTION_OK) {
        return 0;
    }
    
    fprintf(stderr, "CONNECTION_BAD %s\n", PQerrorMessage(conn));

    _stratosphere_disconnect();

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
    message->rcode = atoi(PQgetvalue(res, row, 7));
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
    if (!_stratosphere_connect()) {
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

void fetch_window(const __Source* source, uint64_t fn_req_min, uint64_t fn_req_max, PGresult** pgresult, int32_t* nrows) {
    char sql[1000];
    int pgresult_binary = 1;

    sprintf(sql,
            "SELECT M.ID, FN_REQ, "
                "DN_NN_7.VALUE, "
                "DN_NN_8.VALUE, "
                "DN_NN_9.VALUE, "
                "DN_NN_10.VALUE, "
                "IS_RESPONSE, TOP10M, DYNDNS, M.ID "
            "FROM MESSAGES_%d AS M "
                "JOIN (SELECT * FROM DN_NN WHERE DN_NN.NN_ID = 7) AS DN_NN_7 ON M.DN_ID=DN_NN_7.DN_ID "
                "JOIN (SELECT * FROM DN_NN WHERE DN_NN.NN_ID = 8) AS DN_NN_8 ON M.DN_ID=DN_NN_8.DN_ID "
                "JOIN (SELECT * FROM DN_NN WHERE DN_NN.NN_ID = 9) AS DN_NN_9 ON M.DN_ID=DN_NN_9.DN_ID "
                "JOIN (SELECT * FROM DN_NN WHERE DN_NN.NN_ID = 10) AS DN_NN_10 ON M.DN_ID=DN_NN_10.DN_ID "
                "JOIN DN AS DN ON M.DN_ID=DN.ID "
                "WHERE FN_REQ BETWEEN %ld AND %ld "
            "ORDER BY FN_REQ, M.ID",
            source->id, fn_req_min, fn_req_max
    );

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Select messages error for pcap %d: %s\n", __FILE__, __LINE__, source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);
}

void fetch_source_messages(const __Source* source, int32_t* nrows, PGresult** pgresult) {
    char sql[1000];
    int pgresult_binary = 1;

    printf("id: %d\n", source->id);
    
    sprintf(sql,
            "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS, M.ID, RCODE FROM MESSAGES_%d AS M "
            "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            "JOIN DN AS DN ON M.DN_ID=DN.ID "
            "ORDER BY FN_REQ, ID",
            source->id
            );

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Select messages error for pcap %d: %s\n", __FILE__, __LINE__, source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
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

// void perform_windowingap(RSource source, MANY(PSet) psets, MANY(RWindow) windows_pset[psets.number], int32_t loaded[psets.number]) {
void stratosphere_apply(MANY(RWindowing) windowings, TCPC(MANY(PSet)) psets, int32_t loaded[]) {
    const RSource source = windowings._[0]->source;
    
    PGresult* pgresult = NULL;
    int nrows;

    if (_stratosphere_connect()) {
        return;
    }

    fetch_source_messages(source, &nrows, &pgresult);

    for (int r = 0; r < nrows; r++) {
        DNSMessage message;
        parse_message(pgresult, r, &message);

        for (size_t i = 0; i < windowings.number; i++) {
            RWindowing windowing = windowings._[i];

            const int wnum = (int64_t) floor(message.fn_req / windowing->wsize.value);

            windowing->windows._[wnum]->applies_number = psets->number;

            wapply_run_many(&windowing->windows._[wnum]->applies, &message, psets);
        }
    }

    PQclear(pgresult);

    _stratosphere_disconnect();
}

void stratosphere_run_windowfetch(RWindow0 window0, MANY(PSet) psets) {
    PGresult* pgresult = NULL;
    int nrows;

    if (_stratosphere_connect()) {
        return;
    }

    fetch_window(window0->windowing->source, window0->fn_req_min, window0->fn_req_max, &pgresult, &nrows);

    for (int r = 0; r < nrows; r++) {
        DNSMessage message;
        parse_message(pgresult, r, &message);
        wapply_run_many(&window0->applies, &message, &psets);
    }

    PQclear(pgresult);
}

void _stratosphere_add(TestBed2* tb2) {
    PGresult* pgresult = NULL;

    pgresult = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r, fnreq_max FROM pcap JOIN malware as mw ON malware_id = mw.id ORDER BY qr ASC");

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("[%s:%d] Get pcaps failed: %s\n", __FILE__, __LINE__, PQerrorMessage(conn));
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    nrows = 5; // DEBUG DEVELOP

    for(int row = 0; row < nrows; row++) {
        int32_t id;
        int32_t dgaclass;
        int32_t qr;
        int32_t q;
        int32_t r;
        int32_t fnreq_max;

        int z = 0;
        id = atoi(PQgetvalue(pgresult, row, z++));
        dgaclass = atoi(PQgetvalue(pgresult, row, z++));
        qr = atoi(PQgetvalue(pgresult, row, z++));
        q = atoi(PQgetvalue(pgresult, row, z++));
        r = atoi(PQgetvalue(pgresult, row, z++));
        fnreq_max = atoi(PQgetvalue(pgresult, row, z++));

        RSource rsource = sources_alloc();

        rsource->id = id;
        sprintf(rsource->galaxy, "%s", GALAXY_NAME);
        sprintf(rsource->name, "%s_%d", GALAXY_NAME, id);
        rsource->wclass.bc = dgaclass > 0;
        rsource->wclass.mc = dgaclass;
        rsource->qr = qr;
        rsource->q = q;
        rsource->r = r;
        rsource->fnreq_max = fnreq_max;
        rsource->capture_type = CAPTURETYPE_PCAP;

        testbed2_source_add(tb2, rsource);
    }

    PQclear(pgresult);
}

void stratosphere_add(TestBed2* tb2) {
    if (_stratosphere_connect()) {
        printf("Stratosphere: cannot connect to database.");
        return;
    }

    _stratosphere_add(tb2);

    _stratosphere_disconnect();
}