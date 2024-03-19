#include "trainer_detections.h"

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

void td_io_detection(IOReadWrite rw, char fpath[PATH_MAX], RTrainer trainer, TrainerBy_split* split) {
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
                FRW(dets[i]->dn_count);
                FRW(dets[i]->dn_whitelistened_count);
                FRW(dets[i]->alarms);
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

void* _td_producer(void* argsvoid) {
    struct td_producer_args* args = argsvoid;

    const RTrainer trainer = args->trainer;
    TB2D tb2d = *args->trainer->tb2d;

    size_t total_splits = 0;
    BY_FOR(args->trainer->by, fold) {
        BY_FOR(args->trainer->by, try) {
            TCPC(DatasetSplits) splits = &BY_GET2(tb2d, try, fold);
            for (size_t idxsplit = 0; idxsplit < splits->splits.number; idxsplit++) {
                total_splits++;
            }
        }
    }

    int qm_id = 0;
    BY_FOR(args->trainer->by, fold) {
        BY_FOR(args->trainer->by, try) {
            TCPC(DatasetSplits) splits = &BY_GET2(tb2d, try, fold);

            if (!splits->isok) {
                BY_GET2(trainer->by, fold, try).isok = 0;
                LOG_WARN("Fold [fold=%ld, try=%ld] is not healthy.", idxfold, idxtry);
                continue;
            }

            BY_GET2(trainer->by, fold, try).isok = 1;

            for (size_t idxsplit = 0; idxsplit < splits->splits.number; idxsplit++) {
                char fpath[PATH_MAX];
                int file_exists;
                TD_IO_EXISTS(fpath, file_exists);
                

                if (file_exists) {;
                    LOG_INFO("Result for %ld,%ld,%ld exists.", idxfold, idxtry, idxsplit);

                    td_io_detection(IO_READ, fpath, trainer, &BY_GET3(trainer->by, fold, try, split));

                    continue;
                }
                
                tdqueue_data* qm = calloc(1, sizeof(tdqueue_data));

                qm->id = qm_id++;

                qm->n_configs = trainer->tb2d->tb2w->configsuite.configs.number;

                qm->split = splits->splits._[idxsplit];

                qm->idxfold = idxfold;
                qm->idxtry = idxtry;
                qm->idxsplit = idxsplit;

                tdqueue_enqueue(args->queue, qm, 0);

                LOG_TRACE("Enqueued split %ld/%ld", idxsplit, total_splits);
            }
        }
    }

    tdqueue_end(args->queue);

    return NULL;
}

typedef struct MinMaxIdx {
    size_t min;
    size_t max;
} MinMaxIdx;

MAKEMANY(MinMaxIdx);

void* _td_consumer(void* argsvoid) {
    struct td_consumer_args* args = argsvoid;

    RTrainer trainer = args->trainer;
    const size_t n_configs = trainer->tb2d->tb2w->configsuite.configs.number;
    MANY(Performance) thchoosers = trainer->thchoosers;
    Reducer ths = args->ths;
    const size_t blockconfig_step = args->blockconfig_step;
    MANY(MinMaxIdx) minmax;

    MANY_INIT(minmax, n_configs, MinMax);

    #define FOR_CONFIG for (size_t idxconfig = 0; idxconfig < idxconfigsize; idxconfig++)

    MANY(Detection) detections[trainer->by.n.config];
    MANY(Detection) best_detections[n_configs];
    int best_detections_init[n_configs][trainer->thchoosers.number];
    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        MANY_INIT(detections[idxconfig], ths.many.number, Detection);
        MANY_INIT(best_detections[idxconfig], thchoosers.number, Detection);
    }

    while(!tdqueue_isend(args->queue)) {
        tdqueue_data* qm = tdqueue_dequeue(args->queue);
        
        if (qm == NULL) break;

        // CLOCK_START(_td_consumer);

        const size_t idxtry = qm->idxtry;
        const size_t idxfold = qm->idxfold;
        const size_t idxsplit = qm->idxsplit;

        DatasetSplit split = qm->split;

        {
            size_t avg_n = 0;
            size_t min_idx;
            size_t max_idx;
            size_t min_n = SIZE_MAX;
            size_t max_n = 0;
            for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
                int min_found = 0;
                int max_found = 0;
                const int64_t reduced_min = reducer_logit(split.train->minmax._[idxconfig].min, ths.reducer);
                const int64_t reduced_max = reducer_logit(split.train->minmax._[idxconfig].max, ths.reducer);
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

        for (size_t idxconfigstart = 0; idxconfigstart < n_configs; idxconfigstart += blockconfig_step) {
            size_t idxconfigend = idxconfigstart + blockconfig_step;
            if (idxconfigend > n_configs) idxconfigend = n_configs;
            const size_t idxconfigsize = idxconfigend - idxconfigstart;

            LOG_TRACE("Consumer#%d - [%7ld, %7ld, %7ld] - start train", args->id, idxconfigstart, idxconfigend, n_configs);

            memset(best_detections_init, 0, sizeof(int) * n_configs * trainer->thchoosers.number);
            for (size_t idxconfig = 0; idxconfig < idxconfigsize; idxconfig++) {
                memset(detections[idxconfig]._, 0, detections[idxconfig].number * sizeof(Detection));
                memset(best_detections[idxconfig]._, 0, best_detections[idxconfig].number * sizeof(Detection));
            }

            int progress_print = 0;
            for (size_t idxwindow = 0; idxwindow < split.train->windows.all.number; idxwindow++) {
                RWindow window0 = split.train->windows.all._[idxwindow];
                RSource source = window0->windowing->source;

                FOR_CONFIG {
                    size_t const apply_idxconfig = idxconfig + idxconfigstart;
                    const size_t idxth_start = minmax._[idxconfig].min;
                    const size_t idxth_end = minmax._[idxconfig].max;
                    WApply * const apply = &window0->applies._[apply_idxconfig];
                    MANY(Detection) const * const detections_config = &detections[idxconfig];
                    for (size_t idxth = idxth_start; idxth < idxth_end; idxth++) {
                        detect_run(apply, window0->windowing->source, ths.many._[idxth], &detections_config->_[idxth]);
                    }
                }
                if (idxwindow % (split.train->windows.all.number / 10) == 0) {
                    LOG_TRACE("Consumer#%d - Window#%7ld%% - done", args->id, 10 * (idxwindow / (split.train->windows.all.number / 10)));
                }
            } // calculating detections

            LOG_TRACE("Consumer#%d - [%7ld, %7ld, %7ld] - find better", args->id, idxconfigstart, idxconfigend, n_configs);

            FOR_CONFIG {
                size_t apply_idxconfig = idxconfig + idxconfigstart;
                BY_FOR(trainer->by, thchooser) {
                    for (size_t idxth = minmax._[idxconfig].min; idxth < minmax._[idxconfig].max; idxth++) {
                        double current_score = detect_performance(&detections[idxconfig]._[idxth], &thchoosers._[idxthchooser]);

                        int is_better = 0;
                        if (best_detections_init[idxconfig][idxthchooser] == 1) {
                            double best_score = detect_performance(&best_detections[idxconfig]._[idxthchooser], &thchoosers._[idxthchooser]);
                            is_better = detect_performance_compare(&thchoosers._[idxthchooser], current_score, best_score);
                        } else {
                            is_better = 1;
                        }

                        if (is_better) {
                            best_detections_init[idxconfig][idxthchooser] = 1;
                            memcpy(&best_detections[idxconfig]._[idxthchooser], &detections[idxconfig]._[idxth], sizeof(Detection));
                        }
                    }
                }
            }// calculating performances for each detection and setting the best one

            LOG_TRACE("Consumer#%d - [%7ld, %7ld, %7ld] - start test", args->id, idxconfigstart, idxconfigend, n_configs);

            for (size_t w = 0; w < split.test->windows.all.number; w++) {
                RWindow window0 = split.test->windows.all._[w];
                RSource source = window0->windowing->source;

                FOR_CONFIG {
                    size_t apply_idxconfig = idxconfig + idxconfigstart;
                    BY_FOR(trainer->by, thchooser) {
                        detect_run(
                            &window0->applies._[apply_idxconfig],
                            window0->windowing->source,
                            best_detections[idxconfig]._[idxthchooser].th,
                            &BY_GET4(trainer->by, fold, try, split, thchooser).byconfig._[apply_idxconfig].best_test
                        );

                    }
                }
                
                FOR_CONFIG {
                    size_t apply_idxconfig = idxconfig + idxconfigstart;
                    BY_FOR(trainer->by, thchooser) {
                        TrainerBy_config* result = &BY_GET4(trainer->by, fold, try, split, thchooser).byconfig._[apply_idxconfig];

                        result->threshold_chooser = &thchoosers._[idxthchooser];

                        memcpy(&result->best_train, &best_detections[idxconfig]._[idxthchooser], sizeof(Detection));
                    }
                }
            } // filling Result
        }

        {
            char fpath[PATH_MAX];
            TD_IO_FPATH(fpath);
            td_io_detection(IO_WRITE, fpath, trainer, &BY_GET3(trainer->by, fold, try, split));
        }

        free(qm);
        // CLOCK_END(_td_consumer);
    }

    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        MANY_FREE(best_detections[idxconfig]);
        MANY_FREE(detections[idxconfig]);
    }

    MANY_FREE(minmax);

    return NULL;
}

trainer_detections_context* trainer_detections_start(RTrainer trainer) {
    trainer_detections_context* context = calloc(1, sizeof(trainer_detections_context));

    const size_t nblocks = 50;
    const size_t nlogits_max = 200;
    const int64_t reducer = 100;

    Reducer logits = reducer_run(trainer->tb2d->tb2w, nblocks, nlogits_max, reducer);

    {
        io_path_concat(trainer->tb2d->tb2w->rootdir, "trainer/", trainer->rootdir);
        if (!io_direxists(trainer->rootdir) && io_makedirs(trainer->rootdir)) {
            LOG_ERROR("Impossible to create the directory: %s", trainer->rootdir);
            return NULL;
        }
    }

    // size_t a = 1000;
    // BY_SETN(trainer->by, config, a);
    // trainer->tb2d->tb2w->configsuite.configs.number = a;

    size_t n_configs = trainer->tb2d->tb2w->configsuite.configs.number;
    size_t blockconfig_step = (9e9 / TD_NTHREADS) / (sizeof(Detection) * logits.many.number);
    if (blockconfig_step > n_configs) blockconfig_step = n_configs;

    const int buffer_size = 100;
    context->qm = calloc(buffer_size, sizeof(tdqueue_data));

    context->logits = logits;

    {
        tdqueue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(tdqueue_t));
    }

    context->producer_args.queue = &context->queue;
    context->producer_args.trainer = trainer;
    context->producer_args.ths = logits;

    pthread_create(&context->producer, NULL, _td_producer, &context->producer_args);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;
        context->consumer_args[i].trainer = trainer;
        context->consumer_args[i].ths = logits;
        context->consumer_args[i].blockconfig_step = blockconfig_step;

        // if (i == 0) {
        //     pthread_create(&context->consumers[i], NULL, _td_consumer_old, &context->consumer_args[i]);
        // } else {
        //     pthread_create(&context->consumers[i], NULL, _td_consumer, &context->consumer_args[i]);
        // }
        pthread_create(&context->consumers[i], NULL, _td_consumer, &context->consumer_args[i]);
    }

    return context;
}

int trainer_detections_wait(trainer_detections_context* context) {
    if (!context) {
        LOG_ERROR("Context is NULL.");
        return -1;
    }

    pthread_join(context->producer, NULL);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        pthread_join(context->consumers[i], NULL);
    }

    FILE* file = fopen("/home/princio/Desktop/trainer.csv", "w");
    if (file) {
        RTrainer trainer = context->producer_args.trainer;
        BY_FOR1(trainer->by, fold) {
            BY_FOR2(trainer->by, fold, try) {
                BY_FOR3(trainer->by, fold, try, split) {
                    BY_FOR4(trainer->by, fold, try, split, thchooser) {
                        BY_FOR5(trainer->by, fold, try, split, thchooser, config) {
                            TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);
                            fprintf(file, "%ld,", idxfold);
                            fprintf(file, "%ld,", idxtry);
                            fprintf(file, "%ld,", idxsplit);
                            fprintf(file, "%s,", trainer->thchoosers._[idxthchooser].name);
                            fprintf(file, "%ld,", idxconfig);
                            Detection* dets[2] = {
                                &result->best_train,
                                &result->best_test,
                            };
                            for (int i = 0; i < 2; i++) {
                                fprintf(file, "%f,", result->best_train.th);
                                DGAFOR(cl) {
                                    fprintf(file, "%d,", result->best_train.windows[cl][0]);
                                    fprintf(file, "%d,", result->best_train.windows[cl][1]);
                                }
                            }
                            fprintf(file, "\n");
                        }
                    }
                }
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
