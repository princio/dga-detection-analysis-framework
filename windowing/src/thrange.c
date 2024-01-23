#include "thrange.h"

#include "trainer.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>

// #define TDQUEUE_DEBUG

typedef struct BestDetection {
    int init;
    Detection detection;
} BestDetection;

void thr_queue_enqueue(thr_queue_t *queue, thr_queue_data* value, int is_last) {
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

thr_queue_data* thr_queue_dequeue(thr_queue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		thr_queue_data* value = queue->qm[queue->out];
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

void thr_queue_end(thr_queue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int thr_queue_isend(thr_queue_t *queue)
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

int thr_queue_size(thr_queue_t *queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int size = queue->size;
	pthread_mutex_unlock(&(queue->mutex));
	return size;
}

void thr_io_detection(IOReadWrite rw, char fpath[PATH_MAX], RTrainer trainer, TrainerBy_split* split) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FILE* file = io_openfile(rw, fpath);
    if (!file) {
        LOG_ERROR("Error opening file %s", fpath);
    }
    LOG_INFO("%s %s", rw == IO_READ ? "Loading" : "Saving", fpath);

    for (size_t idxthchooser = 0; idxthchooser < split->bythchooser.number; idxthchooser++) {
        char flag[IO_FLAG_LEN];
        snprintf(flag, IO_FLAG_LEN, "detection_train_start");
        io_flag(rw, file, flag, __LINE__);
        for (size_t idxconfig = 0; idxconfig < split->bythchooser._[idxthchooser].byconfig.number; idxconfig++) {
            Detection* dets[2] = {
                &split->bythchooser._[idxthchooser].byconfig._[idxconfig].best_train,
                &split->bythchooser._[idxthchooser].byconfig._[idxconfig].best_test
            };

            for (size_t i = 0; i < 2; i++) {
                snprintf(flag, IO_FLAG_LEN, "detection_%s_%ld", trainer->thchoosers._[idxthchooser].name, i);
                io_flag(rw, file, flag, __LINE__);
                FRW(dets[i]->th);
                FRW(dets[i]->sources);
                FRW(dets[i]->windows);
            }
        }
    }

    if (fclose(file)) {
        LOG_ERROR("FClose error: %s", strerror(errno));
    }
}

#define TD_IO_FPATH(fpath) \
sprintf(fpath, "results_%ld_%ld_%ld___%ld_%ld_%ld.bin", args->ths.n_blocks, args->ths.n_logit_max, args->ths.reducer, idxfold, idxtry, idxsplit);\
io_path_concat(trainer->rootdir, fpath, fpath);

#define TD_IO_EXISTS(fpath, V) { TD_IO_FPATH(fpath); V = io_fileexists(fpath); }

void* _thr_producer(void* argsvoid) {
    struct thr_producer_args* args = argsvoid;

    RTB2W tb2w = args->thrange->tb2w;

    int qm_id = 0;
    BY_FOR(tb2w->windowing, source) {
        for (size_t idxconfig = 0; idxconfig < tb2w->configsuite.configs.number; idxconfig += args->configs_per_thread) {
            size_t idxconfig_end = idxconfig + args->configs_per_thread;
            if (idxconfig_end > tb2w->configsuite.configs.number) {
                idxconfig_end = tb2w->configsuite.configs.number;
            }
                
            thr_queue_data* qm = calloc(1, sizeof(thr_queue_data));

            qm->id = qm_id++;

            qm->idxconfig_start = idxconfig;
            qm->idxconfig_end = idxconfig_end;
            qm->idxsource = idxsource;

            thr_queue_enqueue(args->queue, qm, 0);

            LOG_TRACE("Enqueued source#%ld\tconfig#[%ld, %ld)", idxsource, idxconfig, idxconfig_end);
        }
    }

    thr_queue_end(args->queue);

    return NULL;
}

typedef struct MinMaxIdx {
    size_t min;
    size_t max;
} MinMaxIdx;

MAKEMANY(MinMaxIdx);

void* _thr_consumer(void* argsvoid) {
    struct thr_consumer_args* args = argsvoid;

    Reducer ths = args->ths;
    RTB2W tb2w = args->thrange->tb2w;
    MANY(Performance) thchoosers = args->thrange->thchoosers;

    while(!thr_queue_isend(args->queue)) {
        thr_queue_data* qm = thr_queue_dequeue(args->queue);
        
        if (qm == NULL) break;

        CLOCK_START(_thr_consumer);

        MANY(RWindow) windows = tb2w->windowing.bysource._[qm->idxsource]->windows;
        RSource source = tb2w->sources._[qm->idxsource];

        for (size_t idxconfig = qm->idxconfig_start; idxconfig < qm->idxconfig_end; idxconfig++) {
            for (size_t idxth = 0; idxth < args->ths.many.number; idxth++) {
                Detection detection;
                memset(&detection, 0, sizeof(Detection));
                for (size_t idxwindow = 0; idxwindow < windows.number; idxwindow++) {
                    RWindow window0 = windows._[idxwindow];
                    WApply * const apply = &window0->applies._[idxconfig];
                    const double th = ths.many._[idxth];
                    const int prediction = apply->logit >= th;
                    const int infected = window0->windowing->source->wclass.mc > 0;
                    const int is_true = prediction == infected;
                    detection.th = th;
                    detection.windows[source->wclass.mc][is_true]++;
                    detection.sources[source->wclass.mc][source->index.multi][is_true]++;
                }

                for (size_t idxthchooser = 0; idxthchooser < args->thrange->thchoosers.number; idxthchooser++) {
                    double current_score = (double) detection.windows[source->wclass.mc][0] / (detection.windows[source->wclass.mc][0] + detection.windows[source->wclass.mc][1]);
                    args->thrange->by.bythchooser._[idxthchooser].byconfig._[idxconfig].byth._[idxth][source->wclass.mc] = current_score;
                }
                
            }
        } // calculating detections

        free(qm);
        CLOCK_END(_thr_consumer);
    }

    return NULL;
}

thrange_context* thrange_start(RTB2W tb2w, MANY(Performance) thchoosers) {
    thrange_context* context = calloc(1, sizeof(thrange_context));

    const size_t nblocks = 50;
    const size_t nlogits_max = 200;
    const int64_t reducer = 100;

    Reducer logits = reducer_run(tb2w, nblocks, nlogits_max, reducer);

    ThRange* thrange = calloc(1, sizeof(ThRange));
    
    thrange->reducer = logits;
    thrange->tb2w = tb2w;
    MANY_CLONE(thrange->thchoosers, thchoosers);

    {
        BY_SETN(thrange->by, thchooser, thchoosers.number);
        BY_SETN(thrange->by, config, tb2w->configsuite.configs.number);
        BY_SETN(thrange->by, th, logits.many.number);

        BY_INIT1(thrange->by, thchooser, ThRangeBy);
        BY_FOR1(thrange->by, thchooser) {
            BY_INIT2(thrange->by, thchooser, config, ThRangeBy);
            BY_FOR2(thrange->by, thchooser, config) {
                BY_INIT3(thrange->by, thchooser, config, th, ThRangeBy);
            }
        }
    }

    {
        io_path_concat(tb2w->rootdir, "thrange/", thrange->rootdir);
        if (!io_direxists(thrange->rootdir) && io_makedirs(thrange->rootdir)) {
            LOG_ERROR("Impossible to create the directory: %s", thrange->rootdir);
            return NULL;
        }
    }

    size_t configs_per_thread = (tb2w->configsuite.configs.number / THR_NTHREADS);

    const int buffer_size = 100;
    context->qm = calloc(buffer_size, sizeof(thr_queue_data));

    context->logits = logits;

    {
        thr_queue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(thr_queue_t));
    }

    context->thrange = thrange;

    context->producer_args.queue = &context->queue;
    context->producer_args.thrange = context->thrange;
    context->producer_args.configs_per_thread = configs_per_thread;

    pthread_create(&context->producer, NULL, _thr_producer, &context->producer_args);

    for (size_t i = 0; i < THR_NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;
        context->consumer_args[i].ths = logits;
        context->consumer_args[i].thrange = thrange;

        pthread_create(&context->consumers[i], NULL, _thr_consumer, &context->consumer_args[i]);
    }

    return context;
}

int thrange_wait(thrange_context* context) {
    if (!context) {
        LOG_ERROR("Context is NULL.");
        return -1;
    }

    pthread_join(context->producer, NULL);

    for (size_t i = 0; i < THR_NTHREADS; ++ i) {
        pthread_join(context->consumers[i], NULL);
    }

    FILE* file = fopen("/home/princio/Desktop/thrange.csv", "w");
    if (file) {
        ThRange* thrange = context->thrange;
        BY_FOR1(thrange->by, thchooser) {
            BY_FOR2(thrange->by, thchooser, config) {
                const size_t n_steps = 4;
                size_t n[N_DGACLASSES][n_steps];
                memset(n, 0, sizeof(size_t) * n_steps * N_DGACLASSES);
                const double score_step = (double) 1 / n_steps;

                DGAFOR(cl) {
                    BY_FOR3(thrange->by, thchooser, config, th) {
                        double score = BY_GET3(thrange->by, thchooser, config, th)[cl];
                        if (score == 1) {
                            n[cl][n_steps - 1]++;
                        } else {
                            size_t idx = floor(score / score_step);
                            n[cl][idx]++;
                            if (idx >= n_steps) {
                                LOG_ERROR("error");
                            }
                        }
                    }
                }

                fprintf(file, "%ld,", idxthchooser);
                fprintf(file, "%ld,", idxconfig);

                for (size_t i = 0; i < n_steps; i++) {
                    DGAFOR(cl) {
                        fprintf(file, "%f,", (double) n[cl][i] / thrange->by.n.th);
                        // fprintf(file, "%ld,", n[i]);
                    }
                }

                fprintf(file, "\n");
            }
        }
        fclose(file);
    } else {
        LOG_ERROR("Impossible to open file: %s", strerror(errno));
    }

    MANY_FREE(context->logits.many);
    free(context->qm);
    free(context);

    return 0;
}

void thrange_free(ThRange* thrange) {

    BY_FOR1(thrange->by, thchooser) {
        BY_FOR2(thrange->by, thchooser, config) {
            MANY_FREE(BY_GET2(thrange->by, thchooser, config).byth);
        }
        MANY_FREE(BY_GET(thrange->by, thchooser).byconfig);
    }
    MANY_FREE(thrange->by.bythchooser);

    free(thrange);
}
