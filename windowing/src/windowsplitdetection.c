#include "windowsplitdetection.h"

#include "configsuite.h"
#include "io.h"
#include "performance_defaults.h"
#include "reducer.h"
#include "windowing.h"
#include "windowmany.h"
#include "windowmc.h"
#include "windowsplit.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#define TD_NTHREADS 1
#define TDQM_MAX_SIZE 500
#define TDQUEUE_INITIALIZER(buffer, buffer_size) { buffer, buffer_size, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }

typedef struct MinMaxIdx {
    size_t min;
    size_t max;
} MinMaxIdx;

MAKEMANY(MinMaxIdx);

void _windowsplitdetection_io_detection(IOReadWrite rw, FILE* file, Detection* detection);
void _windowsplitdetection_io_path(char path[PATH_MAX], RWindowSplit split);


G2Config g2_config_wsplitdet = {
    .id = G2_WSPLITDET,
    .element_size = 0,
    .size = 0,
    .freefn = NULL,
    .hashfn = NULL,
    .iofn = NULL,
    .printfn = NULL
};

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
};

struct td_consumer_args {
    int id;
    tdqueue_t* queue;
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

        _windowsplitdetection_io_path(path, split);
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

struct ConsumerVars {
    Reducer ths;
    MANY(MinMaxIdx) minmaxidx;
    MANY(Detection)* detections;
    Detection* best_detections;
    int* best_detections_init;

    struct {
        char path[PATH_MAX];
        FILE* file;
        RWindowSplit split;
        Performance thchooser_fpr;
        size_t idxbatchconfigstart;
        size_t idxbatchconfigend;
    } run;
};

typedef struct IdxConfigBatch {
    size_t start;
    size_t end;
    size_t count;
    size_t apply;
} IdxConfigBatch;

void* _windowsplitdetection_consumer(void* argsvoid) {
    struct td_consumer_args* args = argsvoid;

    const size_t N_CONFIG = configsuite.configs.number;

    while(!tdqueue_isend(args->queue)) {
        char path[PATH_MAX];
        tdqueue_data* qm;
        FILE* file;
        Performance thchooser_fpr;
        RWindowSplit split;

        qm = tdqueue_dequeue(args->queue);
        
        if (qm == NULL) break;

        split = qm->split;
        
        // _windowsplitdetection_io_path(path, split);
        // file = io_openfile(IO_WRITE, path);
        // if (!file)
        //     exit(1);

        LOG_TRACE("consumer: processing windowsplit %ld.", split->g2index);

        // CLOCK_START(_windowsplitdetection_consumer);

        for (size_t idxconfig = 0; idxconfig < N_CONFIG; idxconfig++) {
            Detection detection;

            windowsplit_detect(split, idxconfig, &detection);

            // _windowsplitdetection_io_detection(IO_WRITE, file, &detection);

            IndexMC count = windowmany_count(detection.windowmany);
            MinMax train_minmax = windowmany_minmax_config(split->train, idxconfig);

            printf("windowsplit %ld (%d/%d)\n", detection.windowmany->g2index, split->config.k, split->config.k_total);
            for (size_t w = 0; w < split->train->number; w++) {
                assert(split->train->_[w]->windowing->source->wclass.bc == 0);
            }
            
            printf("train:\t#%8ld\t(%6.5g to %6.5g)\n", split->train->number, train_minmax.min, train_minmax.max);
            printf(" test:\t#%8ld", split->test->all->number);
            DGAFOR(mc) {
                if (split->test->multi[mc]->number) {
                    MinMax test_minmax = windowmany_minmax_config(split->test->multi[mc], idxconfig);
                    printf("\t(%6.5g to %-6.5g)", test_minmax.min,  test_minmax.max);
                } else {
                    printf("\t(%6s to %-6s)", "-", "-");
                }
            }
            printf("\n");
            configsuite_print(idxconfig);
            printf("\n");
            for (size_t zz = 0; zz < 2; zz++) {

                {
                    printf("%*s", 22, " ");
                    for (size_t z = 0; z < N_DETZONE; z++) {
                        printf("%*g", -19, detection.zone[zz].th[z]);
                    }
                    printf("\n");
                }
                {
                    printf("%*s", 25, " ");
                    for (size_t z = 0; z < N_DETZONE - 1; z++) {
                        printf("|      zone %2ld     ", z);
                    }
                    printf("|\n");
                }
                DGAFOR(cl) {
                    int p = printf("(%6d %6d) %6ld", detection.windows[cl][0], detection.windows[cl][1], count.multi[cl]);
                    printf("%*s", 25 - p, " ");
                    for (size_t z = 0; z < N_DETZONE - 1; z++) {
                        if (detection.zone[zz].zone[z][cl] == 0) printf("|%*s", 18, " ");
                        else {
                            int p = printf("|        %-5d", detection.zone[zz].zone[z][cl]);
                            printf("%*s", 19 - p, " ");
                        }
                    }
                    printf("|\n");
                }
                {
                    DetectionValue fa, ta;
                    detect_alarms(&detection, zz, &fa, &ta);
                    printf("%5d - %5d --- %g\n", fa, ta, ((double) ta) / (fa + ta));
                    // calcolo degli allarmi relativamente per ogni classe 
                }
            }
        } // filling Result

        // if (io_closefile(file, IO_WRITE, path))
        //     exit(1);

        free(qm);
        // CLOCK_END(_windowsplitdetection_consumer);
    }

    return NULL;
}

void* windowsplitdetection_start() {
    char outdir[PATH_MAX];

    windowsplitdetection_context* context = calloc(1, sizeof(windowsplitdetection_context));

    const size_t nblocks = 50;
    const size_t nlogits_max = 200;
    const int64_t reducer = 100;

    // Reducer logits = reducer_run(nblocks, nlogits_max, reducer);

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

    // size_t blockconfig_step = (9e9 / TD_NTHREADS) / (sizeof(Detection) * logits.many.number);
    // if (blockconfig_step > configsuite.configs.number) blockconfig_step = configsuite.configs.number;

    const int buffer_size = 100;
    context->qm = calloc(buffer_size, sizeof(tdqueue_data));

    // context->logits = logits;

    {
        tdqueue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(tdqueue_t));
    }

    context->producer_args.queue = &context->queue;
    // context->producer_args.ths = logits;

    pthread_create(&context->producer, NULL, _windowsplitdetection_producer, &context->producer_args);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;
        // context->consumer_args[i].ths = logits;
        // context->consumer_args[i].blockconfig_step = blockconfig_step;

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

inline void _windowsplitdetection_io_path(char path[PATH_MAX], RWindowSplit split) {
    sprintf(path, "/trainer/result_split_%ld", split->g2index);
    io_path_concat(windowing_iodir, path, path);
}