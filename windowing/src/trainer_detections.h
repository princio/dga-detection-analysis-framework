
#ifndef __TRAINER_DETECTIONS_H__
#define __TRAINER_DETECTIONS_H__

#include "tb2d.h"

#include <pthread.h>
#include <stdio.h>

#define NTHREADS 8
#define TDQM_MAX_SIZE 500

#define TDQUEUE_INITIALIZER(buffer, buffer_size) { buffer, buffer_size, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }


typedef struct tdqueue_data {
    int id;
    int number;

	size_t n_configs;

	size_t idxfold;
	size_t idxtry;
	size_t idxsplit;

    DatasetSplit split;
} tdqueue_data;

typedef struct tdqueue {
	tdqueue_data **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} tdqueue_t;


struct td_producer_args {
    tdqueue_t* queue;
    RTrainer trainer;
};

struct td_consumer_args {
    int id;
    tdqueue_t* queue;
    RTrainer trainer;
};

typedef struct trainer_detections_context {
	tdqueue_data** qm;
	tdqueue_t queue;
	pthread_t producer;
	pthread_t consumers[NTHREADS];
	struct td_producer_args producer_args;
	struct td_consumer_args consumer_args[NTHREADS];

} trainer_detections_context;

trainer_detections_context* trainer_detections_start(RTrainer trainer);

int trainer_detections_wait(trainer_detections_context*);

#endif