
#include "stratosphere_windowing.h"

#include "stratosphere_window.h"

// #include "logger.h"
#include "configsuite.h"
#include "source.h"
#include "windowmany.h"
#include "windowing.h"
#include "window0many.h"

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

#define SWING_QUEUE_NTHREADS 8
#define QM_MAX_SIZE 50000

typedef struct swing_queue_windowing {
    int id;
    RWindowing windowing;
} swing_queue_windowing;

typedef struct queue {
	swing_queue_windowing **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} swing_queue_t;

struct stratosphere_apply_producer_args {
    swing_queue_t* queue;
};

struct stratosphere_apply_consumer_args {
    int id;
    swing_queue_t* queue;
};

void swing_queue_enqueue(swing_queue_t *queue, swing_queue_windowing* value, int is_last) {
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

swing_queue_windowing* _swing_queue_dequeue(swing_queue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		swing_queue_windowing* value = queue->qm[queue->out];
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

void swing_queue_end(swing_queue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int _swing_queue_isend(swing_queue_t *queue)
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

int swing_queue_size(swing_queue_t *queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int size = queue->size;
	pthread_mutex_unlock(&(queue->mutex));
	return size;
}

int _stratosphere_windowing_connect(PGconn** conn) {
    *conn = PQconnectdb("postgresql://princio:postgres@localhost/dns2");

    if (PQstatus(*conn) == CONNECTION_OK) {
        return 0;
    }
    
    LOG_ERROR("connection error %s.", PQerrorMessage(*conn));

    PQfinish(*conn);
    *conn = NULL;

    return -1;
}

void _stratosphere_windowing_parse_message(PGresult* res, int row, DNSMessageWindowing* message) {
    int cursor = 0;
    memset(message, 0, sizeof(DNSMessageWindowing));

    message->wnum = atoi(PQgetvalue(res, row, cursor++));

    message->message.id = atoi(PQgetvalue(res, row, cursor++));
    message->message.top10m = atoi(PQgetvalue(res, row, cursor++));
    message->message.count = atoi(PQgetvalue(res, row, cursor++));
    message->message.q = atoi(PQgetvalue(res, row, cursor++));
    message->message.r = atoi(PQgetvalue(res, row, cursor++));
    message->message.nx = atoi(PQgetvalue(res, row, cursor++));
    for (size_t nn = 0; nn < N_NN; nn++) {
        message->message.value[nn] = atof(PQgetvalue(res, row,  cursor++));
        message->message.logit[nn] = atof(PQgetvalue(res, row,  cursor++));
    }
    assert(message->message.count == (message->message.q + message->message.r));
}

void _stratosphere_windowing_parse_duration(PGresult* res, int row, DNSMessageDuration* message) {
    int cursor = 0;
    memset(message, 0, sizeof(DNSMessageDuration));

    message->wnum = atoi(PQgetvalue(res, row, cursor++));
    message->fn_req_min = atoi(PQgetvalue(res, row, cursor++));
    message->fn_req_max = atoi(PQgetvalue(res, row, cursor++));
    message->time_s_min = atof(PQgetvalue(res, row, cursor++));
    message->time_s_max = atof(PQgetvalue(res, row, cursor++));
}

void _stratosphere_windowing_fetch(PGconn* conn, const RWindowing windowing, int32_t* nrows, PGresult** pgresult) {
    char sql[2000];
    int pgresult_binary = 1;
    
    sprintf(sql, "SELECT ");
    strcat(sql, "M.WNUM,");
    strcat(sql, "M.DN_ID,");
    strcat(sql, "CASE WHEN WL_NN.RANK_BDN IS NULL THEN -1 ELSE 0 END AS RANK_BDN,");
    strcat(sql, "M.QR AS QR,");
    strcat(sql, "M.Q AS Q,");
    strcat(sql, "M.R AS R,");
    strcat(sql, "M.NX AS NX");

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
            sprintf(tmp, "__M.fn_req / %ld as wnum,", windowing->wsize);
            strcat(sql, tmp);
        }
        strcat(sql, "COUNT(*) AS QR, ");
        strcat(sql, "SUM(CASE WHEN __M.IS_R IS FALSE THEN 1 ELSE 0 END) AS Q, ");
        strcat(sql, "SUM(CASE WHEN __M.IS_R IS TRUE  THEN 1 ELSE 0 END) AS R, ");
        strcat(sql, "SUM(CASE WHEN __M.IS_R IS TRUE AND RCODE = 3 THEN 1 ELSE 0 END) AS NX ");
        {
            char tmp[100];
            sprintf(tmp, "FROM MESSAGE_%d __M ", windowing->source->id);
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
    
    strcat(sql, "GROUP BY ");
    strcat(sql, "M.WNUM, ");
    strcat(sql, "M.DN_ID, ");
    strcat(sql, "RANK_BDN, ");
    strcat(sql, "QR, ");
    strcat(sql, "Q, ");
    strcat(sql, "R, ");
    strcat(sql, "NX");

    for (size_t nn = 0; nn < N_NN; nn++) {
        char tmp[200];
        sprintf(tmp, ",VALUE%ld,LOGIT%ld", nn + 1, nn + 1);
        strcat(sql, tmp);
    }
    strcat(sql, ";");

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    printf("\n%s\n", sql);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("select messages error for pcap %d: %s.", windowing->source->id, PQerrorMessage(conn));
        printf("select messages error for pcap %d: %s.", windowing->source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);
}

void _stratosphere_windowing_durations(PGconn* conn, const RWindowing windowing, int32_t* nrows, PGresult** pgresult) {
    char sql[2000];
    int pgresult_binary = 1;
    
    sprintf(sql,
        "SELECT M.FN_REQ / %ld as WNUM, "
        "MIN(M.fn_req), "
        "MAX(M.fn_req), "
        "MIN(M.TIME_S), "
        "MAX(M.TIME_S) "
        "FROM MESSAGE_%d AS M "
        "GROUP BY WNUM ORDER BY WNUM"
        ";",
        windowing->wsize, windowing->source->id);

    *pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);

    printf("\n%s\n", sql);

    if (PQresultStatus(*pgresult) != PGRES_TUPLES_OK) {
        LOG_ERROR("select durations error for pcap %d: %s.", windowing->source->id, PQerrorMessage(conn));
        printf("select durations error for pcap %d: %s.", windowing->source->id, PQerrorMessage(conn));
        PQclear(*pgresult);
        return;
    }

    *nrows = PQntuples(*pgresult);
}

void stratosphere_windowing_fetch(RWindowing windowing, DNSMessageWindowing** message) {

    PGconn* conn;
    PGresult* pgresult;
    int nrows;

    _stratosphere_windowing_connect(&conn);

    _stratosphere_windowing_fetch(conn, windowing, &nrows, &pgresult);

    *message = calloc(nrows, sizeof(DNSMessageWindowing));

    for (int row = 0; row < nrows; row++) {
        _stratosphere_windowing_parse_message(pgresult, row, &(*message)[row]);
    }

    PQfinish(conn);
}


#define CALC_WNUM(FNREQ, WSIZE) FNREQ / WSIZE

void* _stratosphere_windowing_apply_producer(void* argsvoid) {
    struct stratosphere_apply_producer_args* args = argsvoid;
    __MANY many = g2_array(G2_WING);

    swing_queue_windowing* qm = NULL;
    for (size_t i = 0; i < many.number; i++) {
        RWindowing windowing;

        windowing = (RWindowing) many._[i];

        qm = calloc(1, sizeof(swing_queue_windowing));

        qm->windowing = windowing;

        swing_queue_enqueue(args->queue, qm, 0);
    }

    swing_queue_end(args->queue);

    return NULL;
}

void* _stratosphere_windowing_apply_consumer(void* argsvoid) {
    struct stratosphere_apply_consumer_args* args = argsvoid;

    PGconn* thread_conn;

    if (_stratosphere_windowing_connect(&thread_conn)) {
        printf("[Thread#%d] Impossible to connect to the database.", args->id);
        return NULL;
    }

    while(!_swing_queue_isend(args->queue)) {
        swing_queue_windowing* qm = _swing_queue_dequeue(args->queue);
        if (qm == NULL) break;

        PGresult* pgresult;
        int nrows;
        DNSMessageWindowing message;
        RWindowing windowing;

        pgresult = NULL;
        nrows = 0;
        windowing = qm->windowing;

        _stratosphere_windowing_fetch(thread_conn, windowing, &nrows, &pgresult);

        assert(nrows > 0);

        for (int row = 0; row < nrows; row++) {
            _stratosphere_windowing_parse_message(pgresult, row, &message);

            const RWindow window = &windowing->window0many->_[message.wnum];

            window0many_updatewindow(window, &message.message);

            for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
                wapply_grouped_run(&window->applies._[idxconfig], &message.message, &configsuite.configs._[idxconfig]);
                window->n_message++;
            }
        }

        {
            PGresult* pgresult2;
            int nrows2;
            DNSMessageDuration duration;
            _stratosphere_windowing_durations(thread_conn, windowing, &nrows2, &pgresult2);

            for (int row = 0; row < nrows; row++) {
                RWindow window;

                _stratosphere_windowing_parse_duration(pgresult, row, &duration);

                assert(((size_t) duration.wnum) < windowing->window0many->number);
                
                window = &windowing->window0many->_[duration.wnum];
                window->time_s_start = duration.time_s_min;
                window->time_s_end = duration.time_s_max;
            }
            PQclear(pgresult2);
        }


        { // FOR TESTING
            RWindow window = &windowing->window0many->_[0];
            double llr[4];
            stratosphere_window_llr(window, llr);

            if (fabs(window->applies._[0].logit - llr[configsuite.configs._[0].nn]) > 0.001) {
                printf("%f - %f = %f\n",
                    window->applies._[0].logit,
                    llr[configsuite.configs._[0].nn],
                    window->applies._[0].logit - llr[configsuite.configs._[0].nn]);
                printf("\n");
            }
        }

        PQclear(pgresult);
        free(qm);
    }

    PQfinish(thread_conn);

    return NULL;
}

void stratosphere_windowing_apply() {
    int nrows;

    swing_queue_windowing* qm[1000];
	swing_queue_t queue = QUEUE_INITIALIZER(qm);

    struct stratosphere_apply_producer_args producer_args = {
        .queue = &queue,
    };

    struct stratosphere_apply_consumer_args consumer_args[SWING_QUEUE_NTHREADS];

    pthread_t producer;
    pthread_t consumers[SWING_QUEUE_NTHREADS];

    CLOCK_START(windowing);
    {
        pthread_create(&producer, NULL, _stratosphere_windowing_apply_producer, &producer_args);

        for (size_t i = 0; i < SWING_QUEUE_NTHREADS; ++ i) {
            consumer_args[i].id = i;
            consumer_args[i].queue = &queue;

            pthread_create(&consumers[i], NULL, _stratosphere_windowing_apply_consumer, &consumer_args);
        }

        pthread_join(producer, NULL);

        for (size_t i = 0; i < SWING_QUEUE_NTHREADS; ++ i) {
            pthread_join(consumers[i], NULL);
        }
    }
    CLOCK_END(windowing);

    pthread_mutex_destroy(&queue.mutex);
}