#include "windowsplitdetection.h"

#include "configsuite.h"
#include "io.h"
#include "performance_defaults.h"
#include "reducer.h"
#include "windowing.h"
#include "windowmc.h"
#include "windowsplit.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#define TD_NTHREADS 8
#define TDQM_MAX_SIZE 500
#define TDQUEUE_INITIALIZER(buffer, buffer_size) { buffer, buffer_size, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }

typedef struct MinMaxIdx {
    size_t min;
    size_t max;
} MinMaxIdx;

MAKEMANY(MinMaxIdx);

void _windowsplitdetection_io_detection(IOReadWrite rw, FILE* file, Detection* detection);
void _windowsplitdetection_io_path(char outdir[PATH_MAX], char path[PATH_MAX], RWindowSplit split);

typedef struct tdqueue_data {
    int id;
    RWindowSplit split;
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
	Reducer ths;
    char outdir[PATH_MAX];
};

struct td_consumer_args {
    int id;
    tdqueue_t* queue;
	Reducer ths;
	size_t blockconfig_step;
    char outdir[PATH_MAX];
};

typedef struct windowsplitdetection_context {
	tdqueue_data** qm;
	tdqueue_t queue;
	pthread_t producer;
	pthread_t consumers[TD_NTHREADS];
	struct td_producer_args producer_args;
	struct td_consumer_args consumer_args[TD_NTHREADS];
	Reducer logits;
} windowsplitdetection_context;

void tdqueue_enqueue(tdqueue_t *queue, tdqueue_data* value, int is_last) {
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == queue->capacity)
		pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
	queue->qm[queue->in] = value;
	queue->end = is_last;
#ifdef TDQUEUE_DEBUG
    // printf("enqueue %d\n", value->id);
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

tdqueue_data* tdqueue_dequeue(tdqueue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		tdqueue_data* value = queue->qm[queue->out];
		-- queue->size;
		++ queue->out;
		queue->out %= queue->capacity;
#ifdef TDQUEUE_DEBUG
		printf("dequeue %d\n", value->id);
#endif
		pthread_mutex_unlock(&(queue->mutex));
		pthread_cond_broadcast(&(queue->cond_full));
		return value;
	} else {
#ifdef TDQUEUE_DEBUG
		printf("isover \n");
#endif
		pthread_mutex_unlock(&(queue->mutex));
		return NULL;
	}
}

void tdqueue_end(tdqueue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int tdqueue_isend(tdqueue_t *queue)
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

int tdqueue_size(tdqueue_t *queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int size = queue->size;
	pthread_mutex_unlock(&(queue->mutex));
	return size;
}

void* _windowsplitdetection_producer(void* argsvoid) {
    struct td_producer_args* args = argsvoid;

    __MANY many = g2_array(G2_WSPLIT);

    int qm_id = 0;
    for (size_t i = 0; i < many.number; i++) {
        char path[PATH_MAX];
        RWindowSplit split;

        split = many._[i];

        _windowsplitdetection_io_path(args->outdir, path, split);
        if (io_fileexists(path)) {
            LOG_INFO("Skipping split %ld, file exists.", i);
            continue;
        }

        tdqueue_data* qm = calloc(1, sizeof(tdqueue_data));

        qm->id = qm_id++;

        qm->split = split;

        tdqueue_enqueue(args->queue, qm, 0);
    }

    tdqueue_end(args->queue);

    return NULL;
}

void* _windowsplitdetection_consumer(void* argsvoid) {
    struct td_consumer_args* args = argsvoid;

    const size_t n_configs = configsuite.configs.number;
    Reducer ths = args->ths;
    const size_t blockconfig_step = args->blockconfig_step;
    MANY(MinMaxIdx) minmax;

    minmax.number = 0;
    minmax._ = NULL;

    MANY_INIT(minmax, n_configs, MinMax);

    #define FOR_CONFIG for (size_t idxconfig = 0; idxconfig < idxconfigsize; idxconfig++)

    MANY(Detection) detections[configsuite.configs.number];
    Detection best_detections[n_configs];
    int best_detections_init[n_configs];
    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        MANY_INIT(detections[idxconfig], ths.many.number, Detection);
    }

    while(!tdqueue_isend(args->queue)) {
        char path[PATH_MAX];
        tdqueue_data* qm;
        FILE* file;
        Performance thchooser_fpr;

        qm = tdqueue_dequeue(args->queue);
        
        if (qm == NULL) break;

        RWindowSplit split = qm->split;

        
        _windowsplitdetection_io_path(args->outdir, path, split);
        file = io_openfile(IO_WRITE, path);
        if (!file)
            exit(1);

        // CLOCK_START(_windowsplitdetection_consumer);


        { // calcola minmax bo
            size_t avg_n = 0;
            size_t min_idx;
            size_t max_idx;
            size_t min_n = SIZE_MAX;
            size_t max_n = 0;
            for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
                int min_found = 0;
                int max_found = 0;
                const int64_t reduced_min = reducer_logit(split->train->minmax._[idxconfig].min, ths.reducer);
                const int64_t reduced_max = reducer_logit(split->train->minmax._[idxconfig].max, ths.reducer);
                for (size_t idxth = 0; idxth < ths.many.number; idxth++) {
                    if (min_found && max_found) {
                        size_t n_range = max_idx - min_idx;
                        if (n_range == 0) {
                            LOG_ERROR("Range for config#%-7ld is 0", idxconfig);
                        }
                        if (min_n > n_range) min_n = n_range;
                        if (max_n < n_range) max_n = n_range;
                        avg_n += n_range;
                        break;
                    } else {if (!min_found && ths.many._[idxth] > reduced_min) {
                            min_found = 1;
                            min_idx = idxth == 0 ? idxth : idxth - 1;
                            minmax._[idxconfig].min = min_idx;
                        }
                        if (!max_found && ths.many._[idxth] > reduced_max) {
                            max_found = 1;
                            max_idx = idxth;
                            minmax._[idxconfig].max = max_idx;
                        }
                    }
                }
                if (!min_found || !max_found) {
                    LOG_ERROR("No range found for config#%-7ld", idxconfig);
                }
            }
            PRINTF("min: %8ld\n", min_n);
            PRINTF("max: %8ld\n", max_n);
            PRINTF("avg: %8ld\n", avg_n / minmax.number);
        }

        thchooser_fpr = performance_defaults[PERFORMANCEDEFAULTS_FPR];
        
        for (size_t idxconfigstart = 0; idxconfigstart < n_configs; idxconfigstart += blockconfig_step) {
            size_t idxconfigend = idxconfigstart + blockconfig_step;
            if (idxconfigend > n_configs) idxconfigend = n_configs;
            const size_t idxconfigsize = idxconfigend - idxconfigstart;

            LOG_TRACE("Consumer#%d - [%7ld, %7ld, %7ld] - start train", args->id, idxconfigstart, idxconfigend, n_configs);

            memset(best_detections_init, 0, sizeof(int) * n_configs);
            memset(best_detections, 0, sizeof(Detection) * n_configs);
            for (size_t idxconfig = 0; idxconfig < idxconfigsize; idxconfig++) {
                memset(detections[idxconfig]._, 0, ths.many.number * sizeof(Detection));
            }

            int progress_print = 0;
            for (size_t idxwindow = 0; idxwindow < split->train->number; idxwindow++) {
                RWindow window;
                RSource source;

                window = split->train->_[idxwindow];
                source = window->windowing->source;

                FOR_CONFIG {
                    size_t apply_idxconfig = idxconfig + idxconfigstart;
                    size_t idxth_start = minmax._[idxconfig].min;
                    size_t idxth_end = minmax._[idxconfig].max;
                    WApply * apply = &window->applies._[apply_idxconfig];

                    MANY(Detection) * detections_config = &detections[idxconfig];
                    for (size_t idxth = idxth_start; idxth < idxth_end; idxth++) {
                        detect_run(apply, window->windowing->source, ths.many._[idxth], &detections_config->_[idxth]);
                    }
                }
                if (idxwindow % (split->train->number / 10) == 0) {
                    LOG_TRACE("Consumer#%d - Window#%7ld%% - done", args->id, 10 * (idxwindow / (split->train->number / 10)));
                }
            } // calculating detections

            LOG_TRACE("Consumer#%d - [%7ld, %7ld, %7ld] - find better", args->id, idxconfigstart, idxconfigend, n_configs);

            FOR_CONFIG {
                size_t apply_idxconfig = idxconfig + idxconfigstart;
                for (size_t idxth = minmax._[idxconfig].min; idxth < minmax._[idxconfig].max; idxth++) {
                    double current_score = detect_performance(&detections[idxconfig]._[idxth], &thchooser_fpr);

                    int is_better = 0;
                    if (best_detections_init[idxconfig] == 1) {
                        double best_score = detect_performance(&best_detections[idxconfig], &thchooser_fpr);
                        is_better = detect_performance_compare(&thchooser_fpr, current_score, best_score);
                    } else {
                        is_better = 1;
                    }

                    if (is_better) {
                        best_detections_init[idxconfig] = 1;
                        memcpy(&best_detections[idxconfig], &detections[idxconfig]._[idxth], sizeof(Detection));
                    }
                }
            }// calculating performances for each detection and setting the best one

            LOG_TRACE("Consumer#%d - [%7ld, %7ld, %7ld] - start test", args->id, idxconfigstart, idxconfigend, n_configs);

            for (size_t w = 0; w < split->test->all->number; w++) {
                RWindow window = split->test->all->_[w];
                RSource source = window->windowing->source;

                FOR_CONFIG {
                    size_t apply_idxconfig = idxconfig + idxconfigstart;

                    Detection detection_training_best;
                    Detection detection_test;

                    detect_run(
                        &window->applies._[apply_idxconfig],
                        window->windowing->source,
                        best_detections[idxconfig].th,
                        &detection_training_best
                    );

                    memcpy(&detection_test, &best_detections[idxconfig], sizeof(Detection));

                    _windowsplitdetection_io_detection(IO_WRITE, file, &detection_training_best);
                    _windowsplitdetection_io_detection(IO_WRITE, file, &detection_test);
                }

            } // filling Result
        }

        if (io_closefile(file, IO_WRITE, path))
            exit(1);

        free(qm);
        // CLOCK_END(_windowsplitdetection_consumer);
    }

    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        MANY_FREE(detections[idxconfig]);
    }

    MANY_FREE(minmax);

    return NULL;
}

void* windowsplitdetection_start() {
    char outdir[PATH_MAX];

    windowsplitdetection_context* context = calloc(1, sizeof(windowsplitdetection_context));

    const size_t nblocks = 50;
    const size_t nlogits_max = 200;
    const int64_t reducer = 100;

    Reducer logits = reducer_run(nblocks, nlogits_max, reducer);

    {
        io_path_concat(windowing_iodir, "trainer/", outdir);
        if (!io_direxists(outdir) && io_makedirs(outdir)) {
            LOG_ERROR("Impossible to create the directory: %s", outdir);
            return NULL;
        }
    }

    // size_t a = 1000;
    // BY_SETN(trainer->by, config, a);
    // trainer->tb2d->tb2w->configsuite.configs.number = a;

    size_t blockconfig_step = (9e9 / TD_NTHREADS) / (sizeof(Detection) * logits.many.number);
    if (blockconfig_step > configsuite.configs.number) blockconfig_step = configsuite.configs.number;

    const int buffer_size = 100;
    context->qm = calloc(buffer_size, sizeof(tdqueue_data));

    context->logits = logits;

    {
        tdqueue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(tdqueue_t));
    }

    context->producer_args.queue = &context->queue;
    context->producer_args.ths = logits;
    strcpy(context->producer_args.outdir, outdir);

    pthread_create(&context->producer, NULL, _windowsplitdetection_producer, &context->producer_args);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;
        context->consumer_args[i].ths = logits;
        context->consumer_args[i].blockconfig_step = blockconfig_step;
        strcpy(context->consumer_args[i].outdir, outdir);

        pthread_create(&context->consumers[i], NULL, _windowsplitdetection_consumer, &context->consumer_args[i]);
    }

    return context;
}

void windowsplitdetection_wait(void* context_void) {
    assert(context_void);

    windowsplitdetection_context* context = context_void;

    pthread_join(context->producer, NULL);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        pthread_join(context->consumers[i], NULL);
    }

    // FILE* file = fopen("/home/princio/Desktop/trainer.csv", "w");
    // if (file) {
    //     RTrainer trainer = context->producer_args.trainer;
    //     BY_FOR1(trainer->by, fold) {
    //         BY_FOR2(trainer->by, fold, try) {
    //             BY_FOR3(trainer->by, fold, try, split) {
    //                 BY_FOR4(trainer->by, fold, try, split, thchooser) {
    //                     BY_FOR5(trainer->by, fold, try, split, thchooser, config) {
    //                         TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);
    //                         fprintf(file, "%ld,", idxfold);
    //                         fprintf(file, "%ld,", idxtry);
    //                         fprintf(file, "%ld,", idxsplit);
    //                         fprintf(file, "%s,", windowing_thchooser._[idxthchooser].name);
    //                         fprintf(file, "%ld,", idxconfig);
    //                         Detection* dets[2] = {
    //                             &result->best_train,
    //                             &result->best_test,
    //                         };
    //                         for (int i = 0; i < 2; i++) {
    //                             fprintf(file, "%f,", result->best_train.th);
    //                             DGAFOR(cl) {
    //                                 fprintf(file, "%d,", result->best_train.windows[cl][0]);
    //                                 fprintf(file, "%d,", result->best_train.windows[cl][1]);
    //                             }
    //                         }
    //                         fprintf(file, "\n");
    //                     }
    //                 }
    //             }
    //         }
    //     }
    //     fclose(file);
    // } else {
    //     LOG_ERROR("Impossible to open file: %s", strerror(errno));
    // }

    MANY_FREE(context->logits.many);
    free(context->qm);
    free(context);
}


void _windowsplitdetection_io_detection(IOReadWrite rw, FILE* file, Detection* detection) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(detection->th);
    FRW(detection->dn_count);
    FRW(detection->dn_whitelistened_count);
    FRW(detection->alarms);
    FRW(detection->sources);
    FRW(detection->windows);
}

inline void _windowsplitdetection_io_path(char outdir[PATH_MAX], char path[PATH_MAX], RWindowSplit split) {
    sprintf(path, "/trainer/result_split_%ld", split->g2index);
    io_path_concat(outdir, "/trainer", path);
}