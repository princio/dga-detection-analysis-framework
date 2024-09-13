#include "windowsplitdetection.h"

#include "configsuite.h"
#include "io.h"
#include "performance_defaults.h"
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

typedef struct _wsplitqueue_data {
    int id;
    RWindowSplit split;
} _wsplitqueue_data;

typedef struct tdqueue {
	_wsplitqueue_data **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} _wsplitqueue_t;

struct td_producer_args {
    _wsplitqueue_t* queue;
    RTrainer trainer;
};

struct td_consumer_args {
    int id;
    _wsplitqueue_t* queue;
};

typedef struct windowsplitdetection_context {
	_wsplitqueue_data** qm;
	_wsplitqueue_t queue;
	pthread_t producer;
	pthread_t consumers[TD_NTHREADS];
	struct td_producer_args producer_args;
	struct td_consumer_args consumer_args[TD_NTHREADS];
} windowsplitdetection_context;

void _wsplitqueue_enqueue(_wsplitqueue_t *queue, _wsplitqueue_data* value, int is_last) {
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

_wsplitqueue_data* _wsplitqueue_dequeue(_wsplitqueue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		_wsplitqueue_data* value = queue->qm[queue->out];
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

void _wsplitqueue_end(_wsplitqueue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int _wsplitqueue_isend(_wsplitqueue_t *queue)
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

int _wsplitqueue_size(_wsplitqueue_t *queue)
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

        _wsplitqueue_data* qm = calloc(1, sizeof(_wsplitqueue_data));

        qm->id = qm_id++;

        qm->split = split;

        _wsplitqueue_enqueue(args->queue, qm, 0);
    }

    _wsplitqueue_end(args->queue);

    return NULL;
}

struct ConsumerVars {
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

    while(!_wsplitqueue_isend(args->queue)) {
        char path[PATH_MAX];
        _wsplitqueue_data* qm;
        FILE* file;
        Performance thchooser_fpr;
        RWindowSplit split;

        qm = _wsplitqueue_dequeue(args->queue);
        
        if (qm == NULL) break;

        split = qm->split;
        
        // _windowsplitdetection_io_path(path, split);
        // file = io_openfile(IO_WRITE, path);
        // if (!file)
        //     exit(1);

        LOG_TRACE("consumer: processing windowsplit %ld.", split->g2index);

        // CLOCK_START(_windowsplitdetection_consumer);

        for (size_t idxconfig = 0; idxconfig < N_CONFIG; idxconfig++) {
            WindowSplitDetection wsd;
            DetectionCount* zones[2] = {
                &wsd.detection.zone.dn,
                &wsd.detection.zone.llr
            };

            memset(&wsd, 0, sizeof(WindowSplitDetection));

            windowsplit_detect(split, idxconfig, &wsd.detection);

            // _windowsplitdetection_io_detection(IO_WRITE, file, &detection);

            const IndexMC test_count = windowmany_count(split->test->all);
            const MinMax train_minmax = windowmany_minmax_config(split->train, idxconfig);

            printf("windowsplit %ld (%d/%d)\tconfig#%ld: ", split->g2index, split->config.k, split->config.k_total, idxconfig);
            for (size_t w = 0; w < split->train->number; w++) {
                assert(split->train->_[w]->windowing->source->wclass.bc == 0);
            }
            configsuite_print(idxconfig);
            printf("\n");
            
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
            for (size_t zz = 0; zz < 2; zz++) {
                IndexMC count;
                memset(&count, 0, sizeof(IndexMC));
                if (zz == 0) {
                    for (size_t i = 0; i < split->test->all->number; i++) {
                        WClass wc = split->test->all->_[i]->windowing->source->wclass;
                        size_t c = split->test->all->_[i]->applies._[idxconfig].wcount;
                        count.all += c;
                        count.binary[wc.bc] += c;
                        count.multi[wc.mc] += c;
                    }
                } else {
                    count = test_count;
                }
                const int pad = 30;
                const int width = 30;
                {
                    printf("%*s", pad, " ");
                    for (size_t z = 0; z < N_DETBOUND; z++) {
                        printf("%*g", -width, (double) wsd.detection.zone.bounds[z]);
                    }
                    printf("\n");
                }
                {
                    printf("%*s", pad, " ");
                    for (size_t z = 0; z < N_DETZONE; z++) {
                        printf("|%*szone %2ld%*s", width / 2 - 4, " ", z, width / 2 - 4, " ");
                    }
                    printf("|\n");
                }
                DGAFOR(cl) {
                    if (zz == 0) {
                        int p = printf("%12.0g", (double) count.multi[cl]);
                        printf("%*s", 50 - p, " ");
                    } else {
                        int p = printf("%2d %s", cl, DGA_CLASSES[cl]);
                        // int p = printf("%2d (%6.2d %6.2d) %6ld", cl, wsd.detection.windows[cl][0], wsd.detection.windows[cl][1], test_count.multi[cl]);
                        printf("%*s", 50 - p, " ");
                    }
                    for (size_t z = 0; z < N_DETZONE; z++) {
                        if (zones[zz]->all._[z][cl] == 0) {
                            printf("|%*s", width - 1, " ");
                        } else {
                            int p = printf("|  %*d  %*.2g", width / 3, zones[zz]->all._[z][cl], width / 3, ((double) zones[zz]->all._[z][cl]) / count.multi[cl]);
                            printf("%*s", width - p, " ");
                        }
                    }
                    printf("|");
                    printf("\n");
                }
                {
                    DetectionValue fa, ta;
                    detect_alarms(&zones[zz]->all, &fa, &ta);
                    printf("%5d - %5d --- %g\n", fa, ta, ((double) ta) / (fa + ta));
                    // calcolo degli allarmi relativamente per ogni classe 
                }
            }
            for (size_t day = 0; day < 7; day++) {
                DetectionZone* dayzone = &wsd.detection.zone.llr.days[day];
                printf("day %ld)\n", day + 1);
                
                for (size_t cl = 0; cl < N_DGACLASSES; cl++) {
                    if (cl == 2 || cl == 1) continue;
                    printf("%50s\t", DGA_CLASSES[cl]);
                    for (size_t z = 0; z < N_DETBOUND; z++) {
                        if (dayzone->_[z][cl] == 0) {
                            printf("\t%3s", "-");
                        } else {
                            printf("\t%3d", dayzone->_[z][cl]);
                        }
                    }
                    printf("\n");
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

    {
        io_path_concat(windowing_iodir, "trainer/", outdir);
        if (!io_direxists(outdir) && io_makedirs(outdir)) {
            LOG_ERROR("Impossible to create the directory: %s", outdir);
            return NULL;
        }
    }

    const int buffer_size = 100;
    context->qm = calloc(buffer_size, sizeof(_wsplitqueue_data));

    {
        _wsplitqueue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(_wsplitqueue_t));
    }

    context->producer_args.queue = &context->queue;

    pthread_create(&context->producer, NULL, _windowsplitdetection_producer, &context->producer_args);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;

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

    free(context->qm);
    free(context);
}


void _windowsplitdetection_io_detection(IOReadWrite rw, FILE* file, Detection* detection) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRWSIZE(detection, sizeof(Detection));
}

inline void _windowsplitdetection_io_path(char path[PATH_MAX], RWindowSplit split) {
    sprintf(path, "/trainer/result_split_%ld", split->g2index);
    io_path_concat(windowing_iodir, path, path);
}