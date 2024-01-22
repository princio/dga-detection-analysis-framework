
#ifndef __THRANGE_H__
#define __THRANGE_H__

#include "tb2d.h"

#include "detect.h"
#include "reducer.h"

#include <pthread.h>
#include <stdio.h>

#define TD_NTHREADS 8
#define TDQM_MAX_SIZE 500

#define TDQUEUE_INITIALIZER(buffer, buffer_size) { buffer, buffer_size, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }


typedef struct thr_queue_data {
    int id;
    int number;

	size_t n_configs;

	size_t idxfold;
	size_t idxtry;
	size_t idxsplit;

    DatasetSplit split;
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
    RTrainer trainer;
	Reducer ths;
};

struct thr_consumer_args {
    int id;
    thr_queue_t* queue;
    RTrainer trainer;
	Reducer ths;
	size_t blockconfig_step;
};

typedef struct thrange_context {
	thr_queue_data** qm;
	thr_queue_t queue;
	pthread_t producer;
	pthread_t consumers[TD_NTHREADS];
	struct thr_producer_args producer_args;
	struct thr_consumer_args consumer_args[TD_NTHREADS];
	Reducer logits;
} thrange_context;


/****************/

typedef double ThRange_Performance_Values;

MAKEMANY(ThRange_Performance_Values);

typedef struct ThRangeBy_config {
    Performance* threshold_chooser;
    MANY(ThRange_Performance_Values) values;
} ThRangeBy_config;

MAKEMANY(ThRangeBy_config);

typedef struct ThRangeBy_thchooser {
    MANY(ThRangeBy_config) byconfig;
} ThRangeBy_thchooser;
MAKEMANY(ThRangeBy_thchooser);

typedef struct ThRangeBy_split {
    MANY(ThRangeBy_thchooser) bythchooser;
} ThRangeBy_split;

MAKEMANY(ThRangeBy_split);

typedef struct ThRangeBy_try {
    int isok;
    MANY(ThRangeBy_split) bysplit;
} ThRangeBy_try;

MAKEMANY(ThRangeBy_try);

typedef struct ThRangeBy_fold {
    MANY(ThRangeBy_try) bytry;
} ThRangeBy_fold;

MAKEMANY(ThRangeBy_fold);

typedef struct ThRangeBy {
    struct {
        size_t fold;
        size_t try;
        size_t thchooser;
        size_t config;
    } n;
    MANY(ThRangeBy_fold) byfold;
} ThRangeBy;

typedef struct __ThRange {
    char rootdir[DIR_MAX];
    RTB2D tb2d;
    MANY(Performance) thchoosers;
	Reducer reducer;
    ThRangeBy by;
} __ThRange;

/****************/

thrange_context* thrange_start(RTrainer trainer);

int thrange_wait(thrange_context*);

#endif