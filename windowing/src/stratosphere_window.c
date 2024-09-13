
#include "stratosphere.h"

// #include "logger.h"
#include "configsuite.h"
#include "source.h"
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

#include <pthread.h>
#include <stdio.h>

#define QM_MAX_SIZE 50000

#define QUEUE_INITIALIZER(buffer) { buffer, sizeof(buffer) / sizeof(buffer[0]), 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }

#define SWQUEUE_NTHREADS 8
#define QM_MAX_SIZE 50000

typedef struct sw_queue_window {
    int id;
    RWindow window;
} sw_queue_window;

typedef struct queue {
	sw_queue_window **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} sw_queue_t;

struct stratosphere_apply_producer_args {
    sw_queue_t* queue;
};

struct stratosphere_apply_consumer_args {
    int id;
    sw_queue_t* queue;
};

void sw_queue_enqueue(sw_queue_t *queue, sw_queue_window* value, int is_last) {
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == queue->capacity)
		pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
	queue->qm[queue->in] = value;
	queue->end = is_last;
#ifdef WQUEUE_DEBUG
	if(is_last) {
		printf("set end to 1\n");
	}
#endif
	++ queue->size;
	++ queue->in;
	queue->in %= queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

sw_queue_window* sw_queue_dequeue(sw_queue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		sw_queue_window* value = queue->qm[queue->out];
		-- queue->size;
		++ queue->out;
		queue->out %= queue->capacity;
		pthread_mutex_unlock(&(queue->mutex));
		pthread_cond_broadcast(&(queue->cond_full));
		return value;
	} else {
		pthread_mutex_unlock(&(queue->mutex));
		return NULL;
	}
}

void sw_queue_end(sw_queue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int sw_queue_isend(sw_queue_t *queue)
{
	int end;
	pthread_mutex_lock(&(queue->mutex));
	end = queue->end && queue->size == 0;
	pthread_mutex_unlock(&(queue->mutex));
	if (end) {
		pthread_cond_broadcast(&(queue->cond_empty));
	}
	return end;
}

int sw_queue_size(sw_queue_t *queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int size = queue->size;
	pthread_mutex_unlock(&(queue->mutex));
	return size;
}

int _stratosphere_window_connect(PGconn** conn) {
    *conn = PQconnectdb("postgresql://princio:postgres@localhost/dns2");

    if (PQstatus(*conn) == CONNECTION_OK) {
        return 0;
    }
    
    LOG_ERROR("connection error %s.", PQerrorMessage(*conn));

    PQfinish(*conn);
    *conn = NULL;

    return -1;
}

void _stratosphere_window_parse_message(PGresult* res, int row, DNSMessageGrouped* message) {
    int cursor = 0;
    memset(message, 0, sizeof(DNSMessageGrouped));
    message->count = atoi(PQgetvalue(res, row, cursor++));
    message->q = atoi(PQgetvalue(res, row, cursor++));
    message->r = atoi(PQgetvalue(res, row, cursor++));
    message->nx = atoi(PQgetvalue(res, row, cursor++));
    message->id = atoi(PQgetvalue(res, row, cursor++));
    message->top10m = atoi(PQgetvalue(res, row, cursor++));
    for (size_t nn = 0; nn < N_NN; nn++) {
        message->value[nn] = atof(PQgetvalue(res, row,  cursor++));
        message->logit[nn] = atof(PQgetvalue(res, row,  cursor++));
    }
    assert(message->count == (message->q + message->r));
}

void _stratosphere_pcap_fetch(PGconn* conn, const RSource source, const WSize wsize, int32_t* nrows, PGresult** pgresult) {
    char sql[2000];
    int pgresult_binary = 1;
    
    strcat(sql, "SELECT ");
    strcat(sql, "COUNT(*), SUM(M.Q) AS Q, SUM(M.R) AS R, SUM(M.NX) AS NX, ");
    strcat(sql, "M.DN_ID, WL_NN.RANK_BDN");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, ", DN_NN%ld.VALUE AS VALUE%ld, DN_NN%ld.LOGIT AS LOGIT%ld ", nn + 1, nn + 1, nn + 1, nn + 1);
        strcat(sql, tmp);
    }

    strcat(sql, " FROM (");
    {
        strcat(sql, "SELECT ");
        strcat(sql, "__M.DN_ID,");
        {
            char tmp[100];
            sprintf(tmp, "__M.fn_req / %ld as wnum,", wsize);
            strcat(sql, tmp);
        }
        strcat(sql, "SUM(CASE WHEN __M.IS_R IS FALSE THEN 1 ELSE 0 END) AS Q, ");
        strcat(sql, "SUM(CASE WHEN __M.IS_R IS TRUE  THEN 1 ELSE 0 END) AS R, ");
        strcat(sql, "SUM(CASE WHEN __M.IS_R IS TRUE AND RCODE = 3 THEN 1 ELSE 0 END) AS NX");
        {
            char tmp[100];
            sprintf(tmp, " MESSAGE_%d __M ", source->id);
            strcat(sql, tmp);
        }
        strcat(sql, "GROUP BY __M.DN_ID, WNUM");
    }
    strcat(sql, ") AS M ");

    strcat(sql, "LEFT JOIN (SELECT * FROM WHITELIST_DN WHERE WHITELIST_ID = 1) AS WL_NN ON M.DN_ID = WL_NN.DN_ID ");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, " JOIN (SELECT * FROM DN_NN WHERE NN_ID = %ld) AS DN_NN%ld ON M.DN_ID = DN_NN%ld.DN_ID ", nn + 1, nn + 1, nn + 1);
        strcat(sql, tmp);
    }
    strcat(sql, " GROUP BY M.DN_ID, NX, WL_NN.RANK_BDN, WNUM");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, ",VALUE%ld,LOGIT%ld", nn + 1, nn + 1);
        strcat(sql, tmp);
    }
    strcat(sql, ";");

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    printf("\n%s\n", sql);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("select messages error for pcap %d: %s.", source->id, PQerrorMessage(conn));
        printf("select messages error for pcap %d: %s.", source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);
}

void _stratosphere_window_fetch(PGconn* conn, const __Source* source, size_t fnreqmin, size_t fnreqmax, int32_t* nrows, PGresult** pgresult) {
    char sql[2000];
    int pgresult_binary = 1;
    
    sprintf(sql, "SELECT ");
    strcat(sql,  "COUNT(*), SUM(M.Q) AS Q, SUM(M.R) AS R, SUM(M.NX) AS NX, ");
    strcat(sql,  "M.DN_ID, WL_NN.RANK_BDN");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, ", DN_NN%ld.VALUE AS VALUE%ld, DN_NN%ld.LOGIT AS LOGIT%ld ", nn + 1, nn + 1, nn + 1, nn + 1);
        strcat(sql, tmp);
    }

    strcat(sql, "FROM (");

    strcat(sql,
        "SELECT "
        "__M.DN_ID,"
        "CASE WHEN __M.IS_R IS FALSE THEN 1 ELSE 0 END AS Q,"
        "CASE WHEN __M.IS_R IS TRUE  THEN 1 ELSE 0 END AS R,"
        "CASE WHEN __M.IS_R IS TRUE  AND RCODE = 3 THEN 1 ELSE 0 END AS NX");
    {
        char tmp[300];
        sprintf(tmp, " FROM MESSAGE_%d __M", source->id);
        strcat(sql, tmp);
    
        sprintf(tmp, " WHERE __M.FN_REQ >= %ld AND __M.FN_REQ < %ld", fnreqmin, fnreqmax);
        strcat(sql, tmp);
    }
    strcat(sql, ") AS M");

    strcat(sql, " LEFT JOIN (SELECT * FROM WHITELIST_DN WHERE WHITELIST_ID = 1) AS WL_NN ON M.DN_ID = WL_NN.DN_ID ");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, " JOIN (SELECT * FROM DN_NN WHERE NN_ID = %ld) AS DN_NN%ld ON M.DN_ID = DN_NN%ld.DN_ID ", nn + 1, nn + 1, nn + 1);
        strcat(sql, tmp);
    }
    strcat(sql, "GROUP BY M.DN_ID, NX, WL_NN.RANK_BDN");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, ",VALUE%ld,LOGIT%ld", nn + 1, nn + 1);
        strcat(sql, tmp);
    }
    strcat(sql, ";");

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    printf("\n%s\n", sql);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("select messages error for pcap %d: %s.", source->id, PQerrorMessage(conn));
        printf("select messages error for pcap %d: %s.", source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);

    printf("stratosphere source: id=%d\tnrows=%d\n", source->id, *nrows);

    if ((*nrows) != source->qr) {
        LOG_WARN("nrows != source->qr:\t%d != %ld.", *nrows, source->qr);

        if ((*nrows) > source->qr) {
            (*nrows) = source->qr; // we may occur in a new unallocated window
        }
    }
}

void stratosphere_window_fetch(RWindow window, DNSMessageGrouped** message) {

    PGconn* conn;
    PGresult* pgresult;
    int nrows;

    _stratosphere_window_connect(&conn);

    _stratosphere_window_fetch(conn, window->windowing->source, window->fn_req_min, window->fn_req_max, &nrows, &pgresult);

    *message = calloc(nrows, sizeof(DNSMessageGrouped));

    for (int row = 0; row < nrows; row++) {
        _stratosphere_window_parse_message(pgresult, row, &(*message)[row]);
    }

    PQfinish(conn);
}

void stratosphere_window_llr(RWindow window, double* llr) {
    PGconn* conn;
    if (_stratosphere_window_connect(&conn)) {
        return;
    }
    
    int pgresult_binary = 1;
    PGresult* pgresult;
    int nrows;

    char sql[2000];

    sprintf(sql, 
    "SELECT"
    "    SUM(DN_NN1.LOGIT) AS LOGIT1,"
    "    SUM(DN_NN2.LOGIT) AS LOGIT2,"
    "    SUM(DN_NN3.LOGIT) AS LOGIT3,"
    "    SUM(DN_NN4.LOGIT) AS LOGIT4"
    "    FROM"
    "    (SELECT * FROM MESSAGE_%d __M WHERE __M.FN_REQ >= %d AND __M.FN_REQ < %d) AS M"
    "    JOIN (SELECT * FROM DN_NN WHERE NN_ID = 1) AS DN_NN1 ON M.DN_ID = DN_NN1.DN_ID"
    "    JOIN (SELECT * FROM DN_NN WHERE NN_ID = 2) AS DN_NN2 ON M.DN_ID = DN_NN2.DN_ID"
    "    JOIN (SELECT * FROM DN_NN WHERE NN_ID = 3) AS DN_NN3 ON M.DN_ID = DN_NN3.DN_ID"
    "    JOIN (SELECT * FROM DN_NN WHERE NN_ID = 4) AS DN_NN4 ON M.DN_ID = DN_NN4.DN_ID"
    ";"
    , window->windowing->source->id, window->fn_req_min, window->fn_req_max);

    printf("\n---------\n%s\n------------\n", sql);

    pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);
    
    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("PQ error: %s\n", PQerrorMessage(conn));
        PQclear(pgresult);
        return;
    }

    nrows = PQntuples(pgresult);

    assert(nrows == 1);

    for (size_t nn = 0; nn < N_NN; nn++) {
        llr[nn] = atof(PQgetvalue(pgresult, 0, nn));
    }

    PQclear(pgresult);
    PQfinish(conn);
}

#define CALC_WNUM(FNREQ, WSIZE) FNREQ / WSIZE

void* stratosphere_window_apply_producer(void* argsvoid) {
    struct stratosphere_apply_producer_args* args = argsvoid;
    __MANY many = g2_array(G2_WING);

    sw_queue_window* qm = NULL;
    for (size_t i = 0; i < many.number; i++) {
        RWindowing windowing;

        windowing = (RWindowing) many._[i];

        for (size_t w = 0; w < windowing->window0many->number; w++) {
            qm = calloc(1, sizeof(sw_queue_window));

            qm->window = &windowing->window0many->_[w];

            sw_queue_enqueue(args->queue, qm, 0);
        }
    }

    sw_queue_end(args->queue);

    return NULL;
}

void* stratosphere_window_apply_consumer(void* argsvoid) {
    struct stratosphere_apply_consumer_args* args = argsvoid;

    PGconn* thread_conn;

    if (_stratosphere_window_connect(&thread_conn)) {
        printf("[Thread#%d] Impossible to connect to the database.", args->id);
        return NULL;
    }

    while(!sw_queue_isend(args->queue)) {
        sw_queue_window* qm = sw_queue_dequeue(args->queue);
        if (qm == NULL) break;

        PGresult* pgresult;
        int nrows;
        DNSMessageGrouped message;
        RWindow window;

        pgresult = NULL;
        nrows = 0;
        window = qm->window;

        assert(window->fn_req_min < window->fn_req_max);

        _stratosphere_window_fetch(thread_conn, window->windowing->source, window->fn_req_min, window->fn_req_max, &nrows, &pgresult);

        assert(nrows > 0);
        assert(window->n_message == 0);

        for (int row = 0; row < nrows; row++) {
            _stratosphere_window_parse_message(pgresult, row, &message);

            for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
                wapply_grouped_run(&window->applies._[idxconfig], &message, &configsuite.configs._[idxconfig]);
                window->n_message++;
            }
        }
        assert(window->n_message > 0);

        double llr[4];
        stratosphere_window_llr(window, llr);

        if (fabs(window->applies._[0].logit - llr[configsuite.configs._[0].nn]) > 0.001) {
            printf("%f - %f = %f\n",
                window->applies._[0].logit,
                llr[configsuite.configs._[0].nn],
                window->applies._[0].logit - llr[configsuite.configs._[0].nn]);
            printf("\n");
        }

        PQclear(pgresult);
        free(qm);
    }

    PQfinish(thread_conn);

    return NULL;
}

void stratosphere_window_apply() {
    int nrows;

    sw_queue_window* qm[1000];
	sw_queue_t queue = QUEUE_INITIALIZER(qm);

    struct stratosphere_apply_producer_args producer_args = {
        .queue = &queue,
    };

    struct stratosphere_apply_consumer_args consumer_args[SWQUEUE_NTHREADS];

    pthread_t producer;
    pthread_t consumers[SWQUEUE_NTHREADS];

    CLOCK_START(memazzo);
    {
        pthread_create(&producer, NULL, stratosphere_window_apply_producer, &producer_args);

        for (size_t i = 0; i < SWQUEUE_NTHREADS; ++ i) {
            consumer_args[i].id = i;
            consumer_args[i].queue = &queue;

            pthread_create(&consumers[i], NULL, stratosphere_window_apply_consumer, &consumer_args);
        }

        pthread_join(producer, NULL);

        for (size_t i = 0; i < SWQUEUE_NTHREADS; ++ i) {
            pthread_join(consumers[i], NULL);
        }
    }
    CLOCK_END(memazzo);

    pthread_mutex_destroy(&queue.mutex);
}