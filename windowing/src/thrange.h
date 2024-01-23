
#ifndef __THRANGE_H__
#define __THRANGE_H__

#include "tb2d.h"

#include "detect.h"
#include "reducer.h"

#include <pthread.h>
#include <stdio.h>

#define THR_NTHREADS 8
#define TDQM_MAX_SIZE 500

#define TDQUEUE_INITIALIZER(buffer, buffer_size) { buffer, buffer_size, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }


/****************/

typedef double ThRangeBy_th[N_DGACLASSES];
MAKEMANY(ThRangeBy_th);

typedef struct ThRangeBy_config {
    MANY(ThRangeBy_th) byth;
} ThRangeBy_config;
MAKEMANY(ThRangeBy_config);

typedef struct ThRangeBy_thchooser {
    MANY(ThRangeBy_config) byconfig;
} ThRangeBy_thchooser;
MAKEMANY(ThRangeBy_thchooser);

typedef struct ThRangeBy {
    struct {
        size_t thchooser;
        size_t config;
        size_t th;
    } n;
    MANY(ThRangeBy_thchooser) bythchooser;
} ThRangeBy;

typedef struct ThRange {
    char rootdir[DIR_MAX];
    RTB2W tb2w;
    MANY(Performance) thchoosers;
	Reducer reducer;
    ThRangeBy by;
} ThRange;

/****************/


typedef struct thr_queue_data {
    int id;
    int number;

	size_t idxsource;

	size_t idxconfig_start;
	size_t idxconfig_end;
	
} thr_queue_data;

typedef struct thr_queue {
	thr_queue_data **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} thr_queue_t;

struct thr_producer_args {
    thr_queue_t* queue;
    ThRange* thrange;
	size_t configs_per_thread;
};

struct thr_consumer_args {
    int id;
    thr_queue_t* queue;
	Reducer ths;
	ThRange* thrange;
	size_t blockconfig_start;
	size_t blockconfig_end;
};

typedef struct thrange_context {
	thr_queue_data** qm;
	thr_queue_t queue;
	pthread_t producer;
	pthread_t consumers[THR_NTHREADS];
	struct thr_producer_args producer_args;
	struct thr_consumer_args consumer_args[THR_NTHREADS];
	Reducer logits;
	ThRange* thrange;
} thrange_context;

thrange_context* thrange_start(RTB2W tb2w, MANY(Performance) thchoosers);

int thrange_wait(thrange_context*);

void thrange_free(ThRange*);

#endif