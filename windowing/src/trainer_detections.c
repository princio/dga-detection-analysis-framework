#include "trainer_detections.h"

#include "trainer.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>

#define TDQUEUE_DEBUG




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

#define TD_IO_FPATH(fpath) \
sprintf(fpath, "results_%ld_%ld_%ld.bin", idxfold, idxtry, idxsplit);\
io_path_concat(trainer->rootdir, fpath, fpath);

#define TD_IO_EXISTS(fpath, V) {\
TD_IO_FPATH(fpath);\
V = io_fileexists(fpath);\
}

int64_t logit_reducer(double logit, const int reducer) {
    return (int64_t) floor(logit) / reducer * reducer;
}

void td_io_detection(IOReadWrite rw, char fpath[PATH_MAX], RTrainer trainer, TrainerBy_split* split) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FILE* file = io_openfile(rw, fpath);
    if (!file) {
        LOG_ERROR("Error opening file %s", fpath);
    }
    if (rw == IO_READ) {
        LOG_INFO("Loading %s", fpath);
    } else {
        LOG_INFO("Saving %s", fpath);
    }

    for (size_t idxthchooser = 0; idxthchooser < split->bythchooser.number; idxthchooser++) {
        char flag[IO_FLAG_LEN];
        snprintf(flag, IO_FLAG_LEN, "detection_train_start");
        io_flag(rw, file, flag, __LINE__);
        for (size_t idxconfig = 0; idxconfig < split->bythchooser._[idxthchooser].byconfig.number; idxconfig++) {
            Detection* detection_train = &split->bythchooser._[idxthchooser].byconfig._[idxconfig].best_test;
            Detection* detection_test = &split->bythchooser._[idxthchooser].byconfig._[idxconfig].best_test;

            char* thchoosername = trainer->thchoosers._[idxthchooser].name;

            FRW(detection_train->th);
            FRW(detection_train->sources);
            FRW(detection_train->windows);

            snprintf(flag, IO_FLAG_LEN, "detection_test_%s", thchoosername);
            io_flag(rw, file, flag, __LINE__);

            FRW(detection_test->th);
            FRW(detection_test->sources);
            FRW(detection_test->windows);

            snprintf(flag, IO_FLAG_LEN, "detection_%s", thchoosername);
            io_flag(rw, file, flag, __LINE__);
        }
    }

    if (fclose(file)) {
        LOG_ERROR("FClose error: %s", strerror(errno));
    }
}

trainer_detections_logits td_minmax(RTB2D tb2d, const size_t nblocks, const size_t nlogits_max, const int64_t reducer) {
    LOG_INFO("Get all Logits with nblocks#%7ld, nlogits_max$%7ld, reducer#%d", nblocks, nlogits_max, reducer);
    if (nlogits_max > nblocks * 2) {
        LOG_ERROR("Block size too large relatively to nlogits_max (%ldx2 > nlogits_max)");
        exit(1);
    }

    trainer_detections_logits logits;
    uint64_t blocks[nblocks];
    size_t count_logits;

    logits.n_blocks = nblocks;
    logits.n_logit_max = nlogits_max;
    logits.reducer = reducer;

    int64_t logit_min;
    int64_t logit_max;

    { // finding number of logits, maximum and minimum of logit
        count_logits = 0;
        logit_min = INT64_MAX;
        logit_max = INT64_MIN;
        BY_FOR(tb2d->tb2w->windowing, source) {
            for (size_t idxwindow = 0; idxwindow < BY_GET(tb2d->tb2w->windowing, source)->windows.number; idxwindow++) {
                for (size_t idxconfig = 0; idxconfig < tb2d->tb2w->configsuite.configs.number; idxconfig++) {
                    const int64_t logit = logit_reducer(BY_GET(tb2d->tb2w->windowing, source)->windows._[idxwindow]->applies._[idxconfig].logit, reducer);

                    if (logit_min > logit) logit_min = logit;
                    if (logit_max < logit) logit_max = logit;

                    count_logits++;
                }
            }
        }
    }

    LOG_INFO("count_logits %7ld", count_logits);

    const int64_t begin = logit_min;
    const int64_t end = logit_max + reducer;
    const int64_t step = (end - begin) / nblocks;

    { // counting the logits for each block
        memset(blocks, 0, sizeof(int64_t) * nblocks);

        BY_FOR(tb2d->tb2w->windowing, source) {
            LOG_TRACE("Counting source#%4ld having %7ld windows", idxsource, BY_GET(tb2d->tb2w->windowing, source)->windows.number);
            for (size_t idxwindow = 0; idxwindow < BY_GET(tb2d->tb2w->windowing, source)->windows.number; idxwindow++) {
                for (size_t idxconfig = 0; idxconfig < tb2d->tb2w->configsuite.configs.number; idxconfig++) {
                    const int64_t logit = logit_reducer(BY_GET(tb2d->tb2w->windowing, source)->windows._[idxwindow]->applies._[idxconfig].logit, reducer);

                    const size_t index = floor(((double) (logit - begin)) / step);

                    if (index >= nblocks) {
                        LOG_ERROR("Index bigger than number of blocks: %ld > %ld", index,  nblocks);
                    }

                    blocks[index]++;
                }
            }
        }
    }

    size_t nlogits_final = 0;
    { // counting new final logit number
        size_t nlogits = 0;
        for (size_t i = 0; i < nblocks; i++) {
            const double block_perc = (double) blocks[i] / count_logits;
            const size_t block_nlogits = block_perc ? ceil(block_perc * nlogits_max) : 1;
            nlogits += block_nlogits;
        }
        memcpy((size_t*) &nlogits_final, &nlogits, sizeof(size_t));
    }

    assert(nlogits_final > 0);

    MANY_INIT(logits.many, nlogits_final, int64_t);

    {
        size_t nlogits = 0;
        for (size_t i = 0; i < nblocks; i++) {
            const int64_t block_begin = begin + step * i;
            const int64_t block_end = block_begin + step;

            const double block_perc = (double) blocks[i] / count_logits;
            const size_t block_nlogits = block_perc ? ceil(block_perc * nlogits_max) : 1;

            PRINTF("[ %8ld to %8ld ) = %8ld  ->  ", block_begin, block_end, blocks[i]);
            PRINTF("%15.12f%% x %ld = %15.4f\n", block_perc, nlogits_max, ceil(block_perc * nlogits_max));

            const int64_t block_step = step / block_nlogits;
            for (size_t kk = 0; kk < block_nlogits; kk++) {
                if (nlogits < nlogits_final) {
                    logits.many._[nlogits] = block_begin + block_step * kk;
                    PRINTF("%8ld -> %ld\n", nlogits, logits.many._[nlogits]);
                    nlogits++;
                } else {
                    LOG_ERROR("Maximum number of logits reached (nlogits > nlogits_final): %ld > %ld", nlogits, nlogits_final);
                }
            }
        }
        if (logits.many.number < nlogits) {
            LOG_WARN("Number of logits not correspond (logits.number <> nlogits): %ld <> %ld", logits.many.number, nlogits);
        }
        logits.many.number = nlogits;
    }

    return logits;
}

MANY(ThsDataset) _td_ths2(RDataset dataset, const size_t n_configs) {
    MANY(ThsDataset) ths;

    MANY_INIT(ths, n_configs, ThsDataset);

    size_t min = SIZE_MAX;
    size_t max = - SIZE_MAX;
    size_t avg = 0;
    int th_reducer;

    th_reducer = 1000;
    // if (dataset->windows.all.number < 10000) th_reducer = 25;
    // else
    // if (dataset->windows.all.number < 1000) th_reducer = 10;
    // else
    // if (dataset->windows.all.number < 100) th_reducer = 5;
    
    for (size_t a = 0; a < n_configs; a++) {
        MANY_INIT(ths._[a], dataset->windows.all.number, ThsDataset);

        size_t n = 0;

        for (size_t w = 0; w < dataset->windows.all.number; w++) {
            int logit;
            int exists;

            logit = ((int) floor(dataset->windows.all._[w]->applies._[a].logit))  / th_reducer * th_reducer;
            exists = 0;

            for (size_t i = 0; i < n; i++) {
                if (ths._[a]._[i] == logit) {
                    exists = 1;
                    break;
                }
            }

            if(!exists) {
                ths._[a]._[n++] = logit;
            }
        }

        ths._[a]._ = realloc(ths._[a]._, n * sizeof(double));
        ths._[a].number = n;

        if (n > max) max = n;
        if (n < min) min = n;
        avg += n;
    }
    
    // printf("Ths number: reducer=%d\twn=%-10ld\tmin=%-10ld\tmax=%-10ld\tavg=%-10ld", th_reducer, dataset->windows.all.number, min, max, avg / n_configs);

    return ths;
}

MANY(double) _td_ths2_byconfig(RDataset dataset, const size_t idxconfig) {
    MANY(double) ths;

    MANY_INIT(ths, dataset->windows.all.number, ThsDataset);

    size_t min = SIZE_MAX;
    size_t max = - SIZE_MAX;
    size_t avg = 0;
    int th_reducer;

    th_reducer = 100;

    size_t n = 0;
    for (size_t idxwindow = 0; idxwindow < dataset->windows.all.number; idxwindow++) {
        int logit;
        int exists;

        logit = ((int) floor(dataset->windows.all._[idxwindow]->applies._[idxconfig].logit))  / th_reducer * th_reducer;
        exists = 0;

        for (size_t idxth = 0; idxth < n; idxth++) {
            if (ths._[idxth] == logit) {
                exists = 1;
                break;
            }
        }

        if(!exists) {
            ths._[n++] = logit;
        }
    }

    ths._ = realloc(ths._, n * sizeof(double));
    ths.number = n;

    return ths;
}

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
                LOG_WARN("Fold [fold=%ld, try=%ld] is not healthy.", idxfold, idxtry);
                continue;
            }

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

void* _td_consumer_old(void* argsvoid) {
    struct td_consumer_args* args = argsvoid;

    RTrainer trainer = args->trainer;
    const size_t n_configs = trainer->tb2d->tb2w->configsuite.configs.number;
    MANY(Performance) thchoosers = trainer->thchoosers;

    while(!tdqueue_isend(args->queue)) {
        tdqueue_data* qm = tdqueue_dequeue(args->queue);

        if (qm == NULL) break;

        const size_t idxtry = qm->idxtry;
        const size_t idxfold = qm->idxfold;
        const size_t idxsplit = qm->idxsplit;

        DatasetSplit split = qm->split;
        MANY(ThsDataset) ths;
        MANY(Detection) detections[trainer->by.n.config];
        MANY(Detection) best_detections[n_configs];
        int best_detections_init[n_configs][trainer->thchoosers.number];

        memset(best_detections_init, 0, sizeof(int) * n_configs * trainer->thchoosers.number);
        ths = _td_ths2(split.train, n_configs);

        CLOCK_START(_td_consumer_old);

        BY_FOR(trainer->by, config) {
            MANY_INIT(detections[idxconfig], ths._[idxconfig].number, Detection);
            MANY_INIT(best_detections[idxconfig], thchoosers.number, Detection);
        }

        for (size_t idxwindow = 0; idxwindow < split.train->windows.all.number; idxwindow++) {
            RWindow window0 = split.train->windows.all._[idxwindow];
            RSource source = window0->windowing->source;


            BY_FOR(trainer->by, config) {
                for (size_t idxth = 0; idxth < ths._[idxconfig].number; idxth++) {
                    double th = ths._[idxconfig]._[idxth];
                    const int prediction = window0->applies._[idxconfig].logit >= th;
                    const int infected = window0->windowing->source->wclass.mc > 0;
                    const int is_true = prediction == infected;
                    detections[idxconfig]._[idxth].th = th;
                    detections[idxconfig]._[idxth].windows[source->wclass.mc][is_true]++;
                    detections[idxconfig]._[idxth].sources[source->wclass.mc][source->index.multi][is_true]++;
                }
            }
        } // calculating detections

        BY_FOR(trainer->by, config) {
            BY_FOR(trainer->by, thchooser) {
                for (size_t idxth = 0; idxth < ths._[idxconfig].number; idxth++) {
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

        for (size_t idxwindow = 0; idxwindow < split.test->windows.all.number; idxwindow++) {
            RWindow window0 = split.test->windows.all._[idxwindow];
            RSource source = window0->windowing->source;

            BY_FOR(trainer->by, config) {
                BY_FOR(trainer->by, thchooser) {
                    TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);
                    Detection* detection = &result->best_test;
                    double th = best_detections[idxconfig]._[idxthchooser].th;

                    const int prediction = window0->applies._[idxconfig].logit >= th;
                    const int infected = window0->windowing->source->wclass.mc > 0;
                    const int is_true = prediction == infected;

                    detection->th = th;
                    detection->windows[source->wclass.mc][is_true]++;
                    detection->sources[source->wclass.mc][source->index.multi][is_true]++;
                }
            }
        } // filling Result

        BY_FOR(trainer->by, config) {
            BY_FOR(trainer->by, thchooser) {
                TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);

                result->threshold_chooser = &thchoosers._[idxthchooser];

                memcpy(&result->best_train, &best_detections[idxconfig]._[idxthchooser], sizeof(Detection));
            }
        }

        {
            char fpath[PATH_MAX];
            TD_IO_FPATH(fpath);
            td_io_detection(IO_WRITE, fpath, trainer, &BY_GET3(trainer->by, fold, try, split));
        }


        BY_FOR(trainer->by, config) {
            MANY_FREE(best_detections[idxconfig]);
            MANY_FREE(detections[idxconfig]);
        }

        for (size_t i = 0; i < ths.number; i++) {
            MANY_FREE(ths._[i]);
        }
        MANY_FREE(ths);
        free(qm);
        CLOCK_END(_td_consumer_old);
    }

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
    trainer_detections_logits ths = args->ths;
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

        CLOCK_START(_td_consumer);

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
                for (size_t idxth = 0; idxth < ths.many.number; idxth++) {
                    if (!min_found && ths.many._[idxth] > logit_reducer(split.train->minmax._[idxconfig].min, ths.reducer)) {
                        min_found = 1;
                        min_idx = idxth == 0 ? idxth : idxth - 1;
                        minmax._[idxconfig].min = min_idx;
                    }
                    if (!max_found && ths.many._[idxth] > logit_reducer(split.train->minmax._[idxconfig].max, ths.reducer)) {
                        max_found = 1;
                        max_idx = idxth;
                        minmax._[idxconfig].max = max_idx;
                    }
                    if (min_found && max_found) {
                        size_t n_range = max_idx - min_idx;
                        if (n_range == 0) {
                            LOG_ERROR("Range for config#%-7ld is 0", idxconfig);
                        }
                        // LOG_INFO("Range for config#%-7ld is: %5ld - %5ld (%5ld)", idxconfig, min_idx, max_idx, max_idx - min_idx);
                        if (min_n > n_range) min_n = n_range;
                        if (max_n < n_range) max_n = n_range;
                        avg_n += n_range;
                        break;
                    }
                }
            }
            printf("min: %8ld\n", min_n);
            printf("max: %8ld\n", max_n);
            printf("avg: %8ld\n", avg_n / minmax.number);
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
                        const double th = ths.many._[idxth];
                        const int prediction = apply->logit >= th;
                        const int infected = window0->windowing->source->wclass.mc > 0;
                        const int is_true = prediction == infected;
                        detections_config->_[idxth].th = th;
                        detections_config->_[idxth].windows[source->wclass.mc][is_true]++;
                        detections_config->_[idxth].sources[source->wclass.mc][source->index.multi][is_true]++;
                    }
                }
                if (idxwindow % (split.train->windows.all.number / 10) == 0) {
                    LOG_TRACE("Consumer#%d - Window#%7ld/%7ld - done", args->id, idxwindow, split.train->windows.all.number);
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
                    TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);
                        Detection* detection = &result->best_test;
                        double th = best_detections[idxconfig]._[idxthchooser].th;

                        const int prediction = window0->applies._[apply_idxconfig].logit >= th;
                        const int infected = window0->windowing->source->wclass.mc > 0;
                        const int is_true = prediction == infected;

                        detection->th = th;
                        detection->windows[source->wclass.mc][is_true]++;
                        detection->sources[source->wclass.mc][source->index.multi][is_true]++;
                    }
                }
                
                FOR_CONFIG {
                    BY_FOR(trainer->by, thchooser) {
                    TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);

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
        CLOCK_END(_td_consumer);
    }

    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        MANY_FREE(best_detections[idxconfig]);
        MANY_FREE(detections[idxconfig]);
    }

    MANY_FREE(minmax);

    return NULL;
}

void* _td_consumer_byconfig(void* argsvoid) {
    struct td_consumer_args* args = argsvoid;

    RTrainer trainer = args->trainer;
    const size_t n_configs = trainer->tb2d->tb2w->configsuite.configs.number;
    MANY(Performance) thchoosers = trainer->thchoosers;
    MANY(int64_t) ths;
    exit(1);

    while(!tdqueue_isend(args->queue)) {
        tdqueue_data* qm = tdqueue_dequeue(args->queue);
        CLOCK_START(_td_consumer_byconfig);

        if (qm == NULL) break;

        const size_t idxtry = qm->idxtry;
        const size_t idxfold = qm->idxfold;
        const size_t idxsplit = qm->idxsplit;

        DatasetSplit split = qm->split;

        MANY(Detection) manydetection_byth;
        BestDetection best_detections[trainer->thchoosers.number];

        BY_FOR(trainer->by, config) {
            memset(best_detections, 0, sizeof(BestDetection) * trainer->thchoosers.number);

            MANY_INIT(manydetection_byth, ths.number, Detection);

            for (size_t idxwindow = 0; idxwindow < split.train->windows.all.number; idxwindow++) {
                RWindow window0 = split.train->windows.all._[idxwindow];
                RSource source = window0->windowing->source;

                for (size_t idxth = 0; idxth < ths.number; idxth++) {
                    double th = ths._[idxth];
                    const int prediction = window0->applies._[idxconfig].logit >= th;
                    const int infected = window0->windowing->source->wclass.mc > 0;
                    const int is_true = prediction == infected;
                    manydetection_byth._[idxth].th = th;
                    manydetection_byth._[idxth].windows[source->wclass.mc][is_true]++;
                    manydetection_byth._[idxth].sources[source->wclass.mc][source->index.multi][is_true]++;
                }
            } // calculating detections

            BY_FOR(trainer->by, config) {
                BY_FOR(trainer->by, thchooser) {
                    for (size_t idxth = 0; idxth < ths.number; idxth++) {
                        double current_score = detect_performance(&manydetection_byth._[idxth], &thchoosers._[idxthchooser]);

                        int is_better = 0;
                        if (best_detections[idxthchooser].init == 1) {
                            double best_score = detect_performance(&best_detections[idxthchooser].detection, &thchoosers._[idxthchooser]);
                            is_better = detect_performance_compare(&thchoosers._[idxthchooser], current_score, best_score);
                        } else {
                            is_better = 1;
                        }

                        if (is_better) {
                            best_detections[idxthchooser].init = 1;
                            memcpy(&best_detections[idxthchooser].detection, &manydetection_byth._[idxth], sizeof(Detection));
                        }
                    }
                }
            }// calculating performances for each detection and setting the best one

            for (size_t w = 0; w < split.test->windows.all.number; w++) {
                RWindow window0 = split.test->windows.all._[w];
                RSource source = window0->windowing->source;

                BY_FOR(trainer->by, thchooser) {
                    TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);
                    Detection* detection = &result->best_test;
                    double th = best_detections[idxthchooser].detection.th;

                    const int prediction = window0->applies._[idxconfig].logit >= th;
                    const int infected = window0->windowing->source->wclass.mc > 0;
                    const int is_true = prediction == infected;

                    detection->th = th;
                    detection->windows[source->wclass.mc][is_true]++;
                    detection->sources[source->wclass.mc][source->index.multi][is_true]++;
                }
                
                BY_FOR(trainer->by, thchooser) {
                    TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);

                    result->threshold_chooser = &thchoosers._[idxthchooser];

                    memcpy(&result->best_train, &best_detections[idxthchooser].detection, sizeof(Detection));
                }
            } // filling Result

            MANY_FREE(manydetection_byth);
        }
    
        {
            char fpath[PATH_MAX];
            TD_IO_FPATH(fpath);
            td_io_detection(IO_WRITE, fpath, trainer, &BY_GET3(trainer->by, fold, try, split));
        }

        free(qm);
        CLOCK_END(_td_consumer_byconfig);
    }

    return NULL;
}

trainer_detections_context* trainer_detections_start(RTrainer trainer) {
    trainer_detections_context* context = calloc(1, sizeof(trainer_detections_context));

    trainer_detections_logits logits = td_minmax(trainer->tb2d, 50, 100, 100);

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

    MANY_FREE(context->logits.many);
    free(context->qm);
    free(context);

    return 0;
}
