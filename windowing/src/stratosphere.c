
#include "stratosphere.h"

// #include "logger.h"
#include "configsuite.h"
#include "tb2w.h"
#include "wqueue.h"
#include "windows.h"

#include <ncurses.h>
#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>

#include <math.h>
#include <pthread.h>
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
    
    LOG_ERROR("connection error %s.", PQerrorMessage(conn));

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
        LOG_WARN("no data.");
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
        LOG_WARN("get MAX(FN_REQ) for source %d failed.", id);
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
        LOG_ERROR("Select messages error for pcap %d: %s.", source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);
}

void fetch_source_messages(const __Source* source, int32_t* nrows, PGresult** pgresult) {
    char sql[1000];
    int pgresult_binary = 1;
    
    sprintf(sql,
            "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS, M.ID, RCODE FROM MESSAGES_%d AS M "
            "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            "JOIN DN AS DN ON M.DN_ID=DN.ID "
            "ORDER BY FN_REQ, ID",
            source->id
            );

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("select messages error for pcap %d: %s.", source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);

    // printf("stratosphere source: id=%d\tnrows=%d\n", source->id, *nrows);

    if ((*nrows) != source->qr) {
        LOG_WARN("nrows != source->qr:\t%d != %ld.", *nrows, source->qr);

        if ((*nrows) > source->qr) {
            (*nrows) = source->qr; // we may occur in a new unallocated window
        }
    }
}

struct stratosphere_apply_producer_args {
    PGresult* pgresult;
    const int nrows;
    queue_t* queue;
    int qm_max_size;
    size_t wsize;
};

struct stratosphere_apply_consumer_args {
    int id;
    RTB2W tb2w;
    RWindowing windowing;
    queue_t* queue;
};

#define CALC_WNUM(FNREQ, WSIZE) FNREQ / WSIZE

void* stratosphere_apply_producer(void* argsvoid) {
    struct stratosphere_apply_producer_args* args = argsvoid;

    DNSMessage window[args->qm_max_size];
    memset(window, 0, sizeof(window));

    int row = 0;
    int window_index = 0;
    int window_wnum = 0;
    int window_wcount = 0;
    queue_messages* qm = NULL;
    while (row < args->nrows) {
        parse_message(args->pgresult, row, &window[window_wcount]);
        const int wnum = CALC_WNUM(window[window_wcount].fn_req, args->wsize);

        if (!qm) {
            qm = calloc(1, sizeof(queue_messages));
        }


        if (window_wcount > args->qm_max_size) {
            printf("window_wcount > args->qm_max_size: %d > %d\n", window_wcount, args->qm_max_size);
            exit(-1);
        }

        const int is_last_message = (row + 1) == args->nrows;
        int is_window_changed = window_wnum != wnum || is_last_message;
        if (is_window_changed) {
            int requires_newqueue = (qm->number + window_wcount) >= args->qm_max_size;
            if (requires_newqueue) {
                queue_enqueue(args->queue, qm, 0);
                qm = calloc(1, sizeof(queue_messages));
            }

            memcpy(&qm->messages[qm->number], window, sizeof(DNSMessage) * window_wcount);

            qm->number += window_wcount;
            if (is_last_message) {
                qm->messages[qm->number++] = window[window_wcount];
                queue_enqueue(args->queue, qm, 1);
                break;
            } else {
                window[0] = window[window_wcount];
            }
            window_wnum = wnum;
            window_wcount = 0;
        }
        
        window_wcount++;
        row++;
    }
    return NULL;
}

void* stratosphere_apply_consumer(void* argsvoid) {
    struct stratosphere_apply_consumer_args* args = argsvoid;

    while(!queue_isend(args->queue)) {
        queue_messages* qm = queue_dequeue(args->queue);
        if (qm == NULL) break;

        for (int i = 0; i < qm->number; i++) {
            const int wnum = CALC_WNUM(qm->messages[i].fn_req, args->windowing->wsize);
            RWindow rwindow = args->windowing->windows._[wnum];
            for (size_t idxconfig = 0; idxconfig < args->tb2w->configsuite.configs.number; idxconfig++) {
                wapply_run(&rwindow->applies._[idxconfig], &qm->messages[i], &args->tb2w->configsuite.configs._[idxconfig]);
            }
        }
        free(qm);
    }

    return NULL;
}

void stratosphere_apply(RTB2W tb2w, RWindowing windowing) {
    const RSource source = windowing->source;
    
    PGresult* pgresult = NULL;
    int nrows;

    if (_stratosphere_connect()) {
        return;
    }

    fetch_source_messages(source, &nrows, &pgresult);

    printf("\nNROWS: %d\n", nrows);

    queue_messages* qm[100];
	queue_t queue = QUEUE_INITIALIZER(qm);
    int qm_max_size;
    const int qm_min_size = windowing->wsize * 3;
    const int ideal_block_size = (nrows + NTHREADS - 1) / NTHREADS;

    if (ideal_block_size < qm_min_size) {
        qm_max_size = qm_min_size;
        printf("QM SIZE Set to MIN:\t%5d\n", qm_max_size);
    } else
    if (ideal_block_size > QM_MAX_SIZE) {
        qm_max_size = QM_MAX_SIZE;
        printf("QM SIZE Set to MAX:\t%5d\n", qm_max_size);
    } else {
        qm_max_size = ideal_block_size;
        printf("QM SIZE Set to IDEAL:\t%5d\n", qm_max_size);
    }

    const int n_blocks = nrows / qm_max_size + (nrows % qm_max_size > 0);

    struct stratosphere_apply_producer_args producer_args = {
        .pgresult = pgresult,
        .nrows = nrows,
        .queue = &queue,
        .qm_max_size = qm_max_size,
        .wsize = windowing->wsize,
    };

    struct stratosphere_apply_consumer_args consumer_args[NTHREADS];

    pthread_t producer;
    pthread_t consumers[NTHREADS];

    CLOCK_START(memazzo);
    {
        pthread_create(&producer, NULL, stratosphere_apply_producer, &producer_args);

        for (size_t i = 0; i < NTHREADS; ++ i) {
            consumer_args[i].id = i;
            consumer_args[i].tb2w = tb2w;
            consumer_args[i].windowing = windowing;
            consumer_args[i].queue = &queue;

            pthread_create(&consumers[i], NULL, stratosphere_apply_consumer, &consumer_args);
        }

        pthread_join(producer, NULL);

        for (size_t i = 0; i < NTHREADS; ++ i) {
            pthread_join(consumers[i], NULL);
        }
    }
    CLOCK_END(memazzo);

    if (0) { //test
        RWindowing windowing_test = windowings_create(windowing->wsize, source);

        CLOCK_START(memazzodavvero);
        for (int r = 0; r < nrows; r++) {
            DNSMessage message;
            parse_message(pgresult, r, &message);

            const int wnum = CALC_WNUM(message.fn_req, windowing_test->wsize);

            int wcount = 0;
            for (size_t idxconfig = 0; idxconfig < tb2w->configsuite.configs.number; idxconfig++) {
                if (windowing_test->windows._[wnum]->applies.size == 0) {
                    MANY_INIT(windowing_test->windows._[wnum]->applies, tb2w->configsuite.configs.number, WApply);
                }
                wapply_run(&windowing_test->windows._[wnum]->applies._[idxconfig], &message, &tb2w->configsuite.configs._[idxconfig]);
                wcount++;
            }
        }
        CLOCK_END(memazzo);
        for (size_t idxwindow = 0; idxwindow < windowing->windows.number; idxwindow++) {
            for (size_t idxconfig = 0; idxconfig < tb2w->configsuite.configs.number; idxconfig++) {
                WApply* wapply1 = & windowing->windows._[idxwindow]->applies._[idxconfig];
                WApply* wapply2 = & windowing_test->windows._[idxwindow]->applies._[idxconfig];
                double l1 = wapply1->logit;
                double l2 = wapply2->logit;
                if ((size_t) (l1 - l2) * 100000000 > 0) {
                    printf("Error for %6ld\t%6ld!! %f - %f = %f\n", idxwindow, idxconfig, l1, l2, l1 - l2);
                    exit(1);
                }
                if (wapply1->wcount != wapply2->wcount) {
                    printf("Error for %6ld\t%6ld!! %d <> %d\n", idxwindow, idxconfig, wapply1->wcount, wapply2->wcount);
                    exit(1);
                }
            }
        }
    }

    PQclear(pgresult);

    _stratosphere_disconnect();

    pthread_mutex_destroy(&queue.mutex);
}

void _stratosphere_add(RTB2W tb2, size_t limit) {
    PGresult* pgresult = NULL;

    pgresult = PQexec(
        conn, 
        "SELECT pcap.id, mw.dga as dga, qr, q, r, fnreq_max "
        "FROM pcap JOIN malware as mw ON malware_id = mw.id "
        "WHERE pcap.qr > 100000"
        "ORDER BY qr ASC"
    );

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("get pcaps failed: %s.", PQerrorMessage(conn));
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    if (limit && nrows > (int) limit) nrows = limit;

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

        tb2w_source_add(tb2, rsource);
    }

    PQclear(pgresult);
}

void stratosphere_add(RTB2W tb2, size_t limit) {
    if (_stratosphere_connect()) {
        LOG_ERROR("cannot connect to database.");
        return;
    }

    _stratosphere_add(tb2, limit);

    _stratosphere_disconnect();
}