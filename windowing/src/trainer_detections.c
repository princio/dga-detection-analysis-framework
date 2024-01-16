#include "trainer_detections.h"

#include "trainer.h"

#include <float.h>
#include <math.h>

#define TDQUEUE_DEBUG

void tdqueue_enqueue(tdqueue_t *queue, tdqueue_data* value, int is_last) {
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == queue->capacity)
		pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
	queue->qm[queue->in] = value;
	queue->end = is_last;
#ifdef TDQUEUE_DEBUG
    printf("enqueue %d\n", value->id);
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

void* _td_producer(void* argsvoid) {
    struct td_producer_args* args = argsvoid;

    const RTrainer trainer = args->trainer;
    TB2D tb2d = *args->trainer->tb2d;

    int qm_id = 0;
    BY_FOR(args->trainer->by, fold) {
        BY_FOR(args->trainer->by, try) {
            TCPC(DatasetSplits) splits = &BY_GET2(tb2d, try, fold);

            if (!splits->isok) {
                LOG_WARN("Fold [fold=%ld, try=%ld] is not healthy.", idxfold, idxtry);
                continue;
            }

            for (size_t idxsplit = 0; idxsplit < splits->splits.number; idxsplit++) {
                tdqueue_data* qm = calloc(1, sizeof(tdqueue_data));

                qm->id = qm_id++;

                qm->n_configs = trainer->tb2d->tb2w->configsuite.configs.number;

                qm->split = splits->splits._[idxsplit];

                qm->idxfold = idxfold;
                qm->idxtry = idxtry;
                qm->idxsplit = idxsplit;

                tdqueue_enqueue(args->queue, qm, 0);
            }
        }
    }

    tdqueue_end(args->queue);

    return NULL;
}

void* _td_consumer(void* argsvoid) {
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

        BY_FOR(trainer->by, config) {
            MANY_INIT(detections[idxconfig], ths._[idxconfig].number, Detection);
            MANY_INIT(best_detections[idxconfig], thchoosers.number, Detection);
        }

        CLOCK_START(calculating_detections_total);
        CLOCK_START(calculating_detections_for_all_ths);
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
        CLOCK_END(calculating_detections_for_all_ths);


        CLOCK_START(calculating_performance_train);
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
        CLOCK_END(calculating_performance_train);

        CLOCK_START(calculating_performance_from_train_th_to_test);
        for (size_t w = 0; w < split.test->windows.all.number; w++) {
            RWindow window0 = split.test->windows.all._[w];
            RSource source = window0->windowing->source;

            BY_FOR(trainer->by, config) {
                // printf("%ld\t%f\n", );

                BY_FOR(trainer->by, thchooser) {
                    TrainerBy_splits* result = &BY_GET4(trainer->by, config, fold, try, thchooser).splits._[qm->idxsplit];
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
            
            BY_FOR(trainer->by, config) {
                BY_FOR(trainer->by, thchooser) {
                    TrainerBy_splits* result = &BY_GET4(trainer->by, config, fold, try, thchooser).splits._[idxsplit];

                    result->threshold_chooser = &thchoosers._[idxthchooser];

                    memcpy(&result->best_train, &best_detections[idxconfig]._[idxthchooser], sizeof(Detection));
                }
            }
        } // filling Result
        CLOCK_END(calculating_performance_from_train_th_to_test);

        BY_FOR(trainer->by, config) {
            FREEMANY(best_detections[idxconfig]);
            FREEMANY(detections[idxconfig]);
        }

        for (size_t i = 0; i < ths.number; i++) {
            FREEMANY(ths._[i]);
        }
        FREEMANY(ths);
        CLOCK_END(calculating_detections_total);
        free(qm);
    }

    return NULL;
}

trainer_detections_context* trainer_detections_start(RTrainer trainer) {
    trainer_detections_context* context = calloc(1, sizeof(trainer_detections_context));

    const int buffer_size = 100;
    context->qm = calloc(buffer_size, sizeof(tdqueue_data));

    {
        tdqueue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(tdqueue_t));
    }

    context->producer_args.queue = &context->queue;
    context->producer_args.trainer = trainer;

    pthread_create(&context->producer, NULL, _td_producer, &context->producer_args);

    for (size_t i = 0; i < NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;
        context->consumer_args[i].trainer = trainer;

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

    for (size_t i = 0; i < NTHREADS; ++ i) {
        pthread_join(context->consumers[i], NULL);
    }

    free(context->qm);
    free(context);

    return 0;
}
