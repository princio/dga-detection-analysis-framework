
#include "stratosphere.h"

// #include "logger.h"
#include "configsuite.h"
#include "source.h"
#include "wqueue.h"
#include "windowmany.h"
#include "windowing.h"

#include <ncurses.h>
#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>

#include <assert.h>
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
    conn = PQconnectdb("postgresql://princio:postgres@localhost/dns2");

    if (PQstatus(conn) == CONNECTION_OK) {
        return 0;
    }
    
    LOG_ERROR("connection error %s.", PQerrorMessage(conn));

    _stratosphere_disconnect();

    return -1;
}

void _stratosphere_parse_message(PGresult* res, int row, DNSMessage* message) {
    int cursor = 0;
    message->fn_req = atoi(PQgetvalue(res, row, cursor++));
    message->is_response = PQgetvalue(res, row, cursor++)[0] == 't';
    message->id = atol(PQgetvalue(res, row,     cursor++));
    message->rcode = atoi(PQgetvalue(res, row,  cursor++));
    message->top10m = atoi(PQgetvalue(res, row, cursor++));
    for (size_t nn = 0; nn < N_NN; nn++) {
        message->value[nn] = atof(PQgetvalue(res, row,  cursor++));
        message->logit[nn] = atof(PQgetvalue(res, row,  cursor++));
    }
}

void _stratosphere_fetch_message(const __Source* source, int32_t* nrows, PGresult** pgresult) {
    char sql[2000];
    int pgresult_binary = 1;
    
    sprintf(sql, "SELECT FN_REQ, IS_R, M.ID, RCODE, WL_NN.RANK_BDN");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, ", DN_NN%ld.VALUE AS VALUE%ld, DN_NN%ld.LOGIT AS LOGIT%ld ", nn + 1, nn + 1, nn + 1, nn + 1);
        strcat(sql, tmp);
    }

    {
        char tmp[100];
        sprintf(tmp, " FROM MESSAGE_%d AS M ", source->id);
        strcat(sql, tmp);
    }

    strcat(sql, " LEFT JOIN (SELECT * FROM WHITELIST_DN WHERE WHITELIST_ID = 1) AS WL_NN ON M.DN_ID = WL_NN.DN_ID ");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, " JOIN (SELECT * FROM DN_NN WHERE NN_ID = %ld) AS DN_NN%ld ON M.DN_ID = DN_NN%ld.DN_ID ", nn + 1, nn + 1, nn + 1);
        strcat(sql, tmp);
    }
    strcat(sql, "ORDER BY FN_REQ, ID;");

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
        _stratosphere_parse_message(args->pgresult, row, &window[window_wcount]);
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
            RWindow rwindow = &args->windowing->window0many->_[wnum];
            for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
                wapply_run(&rwindow->applies._[idxconfig], &qm->messages[i], &configsuite.configs._[idxconfig]);
                rwindow->n_message++;
            }
        }
        free(qm);
    }

    return NULL;
}

void stratosphere_apply(RWindowing windowing) {
    const RSource source = windowing->source;
    
    PGresult* pgresult = NULL;
    int nrows;

    if (_stratosphere_connect()) {
        return;
    }

    _stratosphere_fetch_message(source, &nrows, &pgresult);

    queue_messages* qm[100];
	queue_t queue = QUEUE_INITIALIZER(qm);
    int qm_max_size;
    const int qm_min_size = windowing->wsize * 3;
    const int ideal_block_size = (nrows + WQUEUE_NTHREADS - 1) / WQUEUE_NTHREADS;

    if (ideal_block_size < qm_min_size) {
        qm_max_size = qm_min_size;
    } else
    if (ideal_block_size > QM_MAX_SIZE) {
        qm_max_size = QM_MAX_SIZE;
    } else {
        qm_max_size = ideal_block_size;
    }

    const int n_blocks = nrows / qm_max_size + (nrows % qm_max_size > 0);

    struct stratosphere_apply_producer_args producer_args = {
        .pgresult = pgresult,
        .nrows = nrows,
        .queue = &queue,
        .qm_max_size = qm_max_size,
        .wsize = windowing->wsize,
    };

    struct stratosphere_apply_consumer_args consumer_args[WQUEUE_NTHREADS];

    pthread_t producer;
    pthread_t consumers[WQUEUE_NTHREADS];

    CLOCK_START(memazzo);
    {
        pthread_create(&producer, NULL, stratosphere_apply_producer, &producer_args);

        for (size_t i = 0; i < WQUEUE_NTHREADS; ++ i) {
            consumer_args[i].id = i;
            consumer_args[i].windowing = windowing;
            consumer_args[i].queue = &queue;

            pthread_create(&consumers[i], NULL, stratosphere_apply_consumer, &consumer_args);
        }

        pthread_join(producer, NULL);

        for (size_t i = 0; i < WQUEUE_NTHREADS; ++ i) {
            pthread_join(consumers[i], NULL);
        }
    }
    CLOCK_END(memazzo);

/*
    if (0) { //test
        RWindowing windowing_test = windowings_create(windowing->wsize, source);

        CLOCK_START(memazzodavvero);
        for (int r = 0; r < nrows; r++) {
            DNSMessage message;
            parse_message(pgresult, r, &message);

            const int wnum = CALC_WNUM(message.fn_req, windowing_test->wsize);

            int wcount = 0;
            for (size_t idxconfig = 0; idxconfig < suite.configs.number; idxconfig++) {
                if (windowing_test->windowmany->_[wnum]->applies.size == 0) {
                    MANY_INIT(windowing_test->windowmany->_[wnum]->applies, suite.configs.number, WApply);
                }
                wapply_run(&windowing_test->windowmany->_[wnum]->applies._[idxconfig], &message, &suite.configs._[idxconfig]);
                wcount++;
            }
        }
        CLOCK_END(memazzo);
        for (size_t idxwindow = 0; idxwindow < windowing->windowmany->number; idxwindow++) {
            for (size_t idxconfig = 0; idxconfig < suite.configs.number; idxconfig++) {
                WApply* wapply1 = & windowing->windowmany->_[idxwindow]->applies._[idxconfig];
                WApply* wapply2 = & windowing_test->windowmany->_[idxwindow]->applies._[idxconfig];
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
*/

    PQclear(pgresult);

    _stratosphere_disconnect();

    pthread_mutex_destroy(&queue.mutex);
}

void _stratosphere_add(char dataset[100], size_t limit) {
    PGresult* pgresult = NULL;
    char sql[1000];

    memset(DGA_CLASSES, 0, sizeof(DGA_CLASSES));

    DGAFOR(cl) {
        strcpy(DGA_CLASSES[cl], "freqmax=0");
    }

    sprintf(sql,
        "SELECT "
        "pcap.id, pcap.name, mw.dga as dga, mw.name, qr, q, r, fnreq_max, dga_ratio, day, days "
        "FROM pcap JOIN malware as mw ON malware_id = mw.id "
        "WHERE "
        " pcap.dataset = '%s' AND "
        " FNREQ_MAX > 0 "
        " ORDER BY qr ASC "
    #ifdef DEBUG_TEST
        "LIMIT %d"
    #endif
        ,
        dataset
    #ifdef DEBUG_TEST
        , DEBUG_TEST_STRATOSPHERE_SOURCE_LIMIT
    #endif
    );

    pgresult = PQexec(conn, sql);

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("get pcaps failed: %s.", PQerrorMessage(conn));
        exit(1);
    }

    int nrows = PQntuples(pgresult);

    if (limit && nrows > (int) limit) nrows = limit;

    for(int row = 0; row < nrows; row++) {
        int32_t id;
        int32_t dgaclass;
        char pcapname[200];
        char mwname[200];
        int32_t qr;
        int32_t q;
        int32_t r;
        int32_t fnreq_max;
        float dga_ratio;
        int day, days;

        int z = 0;
        id = atoi(PQgetvalue(pgresult, row, z++));
        strcpy(pcapname, PQgetvalue(pgresult, row, z++));
        dgaclass = atoi(PQgetvalue(pgresult, row, z++));
        strcpy(mwname, PQgetvalue(pgresult, row, z++));
        qr = atoi(PQgetvalue(pgresult, row, z++));
        q = atoi(PQgetvalue(pgresult, row, z++));
        r = atoi(PQgetvalue(pgresult, row, z++));
        fnreq_max = atoi(PQgetvalue(pgresult, row, z++));
        dga_ratio = atof(PQgetvalue(pgresult, row, z++));

        {
            char* _day = PQgetvalue(pgresult, row, z++);
            char* _days = PQgetvalue(pgresult, row, z++);
            day = -1 + (_day ? atoi(_day) : 0);
            days = (_days ? atoi(_days) : -1);
        }

        assert(day >= -1 && day < 7);

        RSource rsource = source_alloc();

        rsource->id = id;
        strcpy(rsource->galaxy, GALAXY_NAME);
        strcpy(rsource->name, pcapname);
        rsource->wclass.bc = dgaclass > 0;
        rsource->wclass.mc = dgaclass; //dgaclass > 0 ? (dga_ratio > 0.3 ? 2 : 1) : 0; // (dgaclass == 0 || dgaclass == 2) ? dgaclass : 1;
        strcpy(rsource->wclass.mwname, mwname);
        strcpy(DGA_CLASSES[rsource->wclass.mc], mwname);
        rsource->qr = qr;
        rsource->q = q;
        rsource->r = r;
        rsource->fnreq_max = fnreq_max;
        rsource->day = day;
        rsource->days = days;
    }

    PQclear(pgresult);
}

void stratosphere_add(char dataset[100], size_t limit) {
    if (_stratosphere_connect()) {
        LOG_ERROR("cannot connect to database.");
        return;
    }

    _stratosphere_add(dataset, limit);

    _stratosphere_disconnect();
}