#include "windowzonedetection.h"

#include "configsuite.h"
#include "io.h"
#include "performance_defaults.h"
#include "windowing.h"
#include "windowmany.h"
#include "windowmc.h"

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

void _windowzonedetection_io_detection(IOReadWrite rw, FILE* file, Detection* detection);
void _windowzonedetection_io_path(char path[PATH_MAX], RWindowMC zone);


G2Config g2_config_wzonedet = {
    .id = G2_WSPLITDET,
    .element_size = 0,
    .size = 0,
    .freefn = NULL,
    .hashfn = NULL,
    .iofn = NULL,
    .printfn = NULL
};

typedef struct _wzonequeue_data {
    int id;
    RWindowMC windowmc;
    size_t idxconfig;
} _wzonequeue_data;

typedef struct tdqueue {
	_wzonequeue_data **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} _wzonequeue_t;

struct td_producer_args {
    _wzonequeue_t* queue;
    RTrainer trainer;
};

struct td_consumer_args {
    int id;
    _wzonequeue_t* queue;
};

typedef struct windowzonedetection_context {
	_wzonequeue_data** qm;
	_wzonequeue_t queue;
	pthread_t producer;
	pthread_t consumers[TD_NTHREADS];
	struct td_producer_args producer_args;
	struct td_consumer_args consumer_args[TD_NTHREADS];
} windowzonedetection_context;

void _wzonequeue_enqueue(_wzonequeue_t *queue, _wzonequeue_data* value, int is_last) {
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

_wzonequeue_data* _wzonequeue_dequeue(_wzonequeue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		_wzonequeue_data* value = queue->qm[queue->out];
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

void _wzonequeue_end(_wzonequeue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int _wzonequeue_isend(_wzonequeue_t *queue)
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

int _wzonequeue_size(_wzonequeue_t *queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int size = queue->size;
	pthread_mutex_unlock(&(queue->mutex));
	return size;
}

void _windowzone_detect(RWindowMC windowmc, size_t const idxconfig, Detection* detection) {
    MinMax logitminmax;
    double thzone[N_DETBOUND];
    double th;

    memset(&logitminmax, 0, sizeof(MinMax));
    memset(thzone, 0, sizeof(thzone));

    logitminmax = windowmany_minmax_config(windowmc->binary[0], idxconfig);
    th = logitminmax.max + 1;

    thzone[0] = - DBL_MAX;
    thzone[N_DETZONE] = DBL_MAX;

    thzone[1] = logitminmax.min;
    thzone[2] = (logitminmax.max + logitminmax.min) / 2;
    thzone[3] = logitminmax.max + 1;

    thzone[4] = thzone[3] + (logitminmax.max - logitminmax.min) * 0.1;

    detect_run(detection, windowmc->all, idxconfig, th, thzone);
}

void* _windowzonedetection_producer(void* argsvoid) {
    struct td_producer_args* args = argsvoid;

    RWindowMC windowmc;

    windowmc = g2_insert_alloc_item(G2_WMC);
    windowmc_init(windowmc);
    windowmc_buildby_windowing(windowmc);

    int qm_id = 0;
    for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
        char path[PATH_MAX];

        _windowzonedetection_io_path(path, windowmc);
        if (io_fileexists(path)) {
            LOG_INFO("Skipping zone %ld, file exists.", idxconfig);
            continue;
        }

        _wzonequeue_data* qm = calloc(1, sizeof(_wzonequeue_data));

        qm->id = qm_id++;

        qm->windowmc = windowmc;
        qm->idxconfig = idxconfig;

        _wzonequeue_enqueue(args->queue, qm, 0);
    }

    _wzonequeue_end(args->queue);

    return NULL;
}

void windowzonedetection_print(Detection* detection) {
    const IndexMC window_count = windowmany_count(detection->windowmany);
    
    IndexMC dn_count;
    {
        memset(&dn_count, 0, sizeof(IndexMC));
        for (size_t i = 0; i < detection->windowmany->number; i++) {
            WClass wc = detection->windowmany->_[i]->windowing->source->wclass;
            size_t c = detection->windowmany->_[i]->applies._[detection->idxconfig].wcount;
            dn_count.all += c;
            dn_count.binary[wc.bc] += c;
            dn_count.multi[wc.mc] += c;
        }
    }

    printf("windowzone %ld \tconfig#%ld: ", detection->windowmany->g2index, detection->idxconfig);
    configsuite_print(detection->idxconfig);
    printf("\n");

    for (size_t z = 0; z < N_DETBOUND; z++) {
        printf("%10.3g\t", WApplyDNBad_Values[z]);
    }
    printf("\n");

    for (size_t z = 0; z < N_DETBOUND; z++) {
        printf("%10.3g\t", detection->zone.bounds[z]);
    }
    printf("\n");

    for (size_t day = 0; day < 7; day++) {
        for (size_t z = 0; z < N_DETBOUND; z++) {
            printf("%10.3g\t", detection->zone.bounds[z]);
        }
        printf("\n");
    }
    
    
    for (size_t day = 0; day < 7; day++) {
        printf("day %ld)\n", day + 1);
        
        DGAFOR(cl) {
            {
                int all0 = 0;
                for (size_t z = 0; z < N_DETZONE; z++) {
                    all0 += DETGETDAY(detection, dn, day, z, cl);
                }
                if (!all0) continue;
            }
            if (cl == 2 || cl == 1) continue;
            printf("%-35s", DGA_CLASSES[cl]);
            uint32_t totdn = 0, totllr = 0;
            for (size_t z = 0; z < N_DETZONE; z++) {
                totdn += DETGETDAY(detection, dn, day, z, cl);
                totllr += DETGETDAY(detection, llr, day, z, cl);
            }
            printf(" (%5u %5u)", totdn, totllr);
            for (size_t z = 0; z < N_DETZONE; z++) {
                if (DETGETDAY(detection, dn, day, z, cl) == 0) {
                    printf("   %4s", "  -");
                } else {
                    printf("   %4u", DETGETDAY(detection, dn, day, z, cl));
                }
            }
            printf(" | ");
            for (size_t z = 0; z < N_DETZONE; z++) {
                if (DETGETDAY(detection, llr, day, z, cl) == 0) {
                    printf("   %4s", "  -");
                } else {
                    printf("   %4u", DETGETDAY(detection, llr, day, z, cl));
                }
            }
            printf("\n");
        }
        printf("\n");
    }
}

void windowzonedetection_csv(FILE* fp, Detection* detection) {
    const IndexMC window_count = windowmany_count(detection->windowmany);
    
    fprintf(fp, "g2index,idxconfig,");
    for (size_t z = 0; z < N_DETBOUND; z++) {
        fprintf(fp, "bound_dn_%ld,", z);
    }
    for (size_t z = 0; z < N_DETBOUND; z++) {
        fprintf(fp, "bound_llr_%ld,", z);
    }

    fprintf(fp, "mwname,");
    fprintf(fp, "dn_all_tot,");
    for (size_t z = 0; z < N_DETZONE; z++) {
        fprintf(fp, "dn_all_z%02ld,", z);
    }
    fprintf(fp, "llr_all_tot,");
    for (size_t z = 0; z < N_DETZONE; z++) {
        fprintf(fp, "llr_all_z%02ld,", z);
    }
    for (size_t day = 0; day < 7; day++) {
        fprintf(fp, "dn_day%ld_tot,", day);
        for (size_t z = 0; z < N_DETZONE; z++) {
            fprintf(fp, "dn_day%ld_z%02ld,", day, z);
        }
        fprintf(fp, "llr_day%ld_tot,", day);
        for (size_t z = 0; z < N_DETZONE; z++) {
            fprintf(fp, "llr_day%ld_z%02ld,", day, z);
        }
    }

    fprintf(fp, "\n");
    
    DGAFOR(cl) {
        fprintf(fp, "%ld,%ld,", detection->g2index, detection->idxconfig);
        for (size_t z = 0; z < N_DETBOUND; z++) {
            fprintf(fp, "%f,", WApplyDNBad_Values[z]);
        }
        for (size_t z = 0; z < N_DETBOUND; z++) {
            fprintf(fp, "%f,", detection->zone.bounds[z]);
        }
        fprintf(fp, "\"%s\",", DGA_CLASSES[cl]);
        {
            uint32_t totdn;
            uint32_t totllr;

            totdn = 0;
            totllr = 0;
            for (size_t z = 0; z < N_DETZONE; z++) {
                totdn += DETGETALL_(dn);
                totllr += DETGETALL_(llr);
            }
        
            fprintf(fp, "%u,", totdn);
            for (size_t z = 0; z < N_DETZONE; z++) {
                fprintf(fp, "%u,", DETGETALL_(dn));
            }

            fprintf(fp, "%u,", totllr);
            for (size_t z = 0; z < N_DETZONE; z++) {
                fprintf(fp, "%u,", DETGETALL_(llr));
            }
        }

        {
            for (size_t day = 0; day < 7; day++) {
                uint32_t totdn = 0;
                uint32_t totllr = 0;
                for (size_t z = 0; z < N_DETZONE; z++) {
                    totdn += DETGETDAY_(dn);
                    totllr += DETGETDAY_(llr);
                }

                fprintf(fp, "%u,", totdn);
                for (size_t z = 0; z < N_DETZONE; z++) {
                    fprintf(fp, "%u,", DETGETDAY_(dn));
                }

                fprintf(fp, "%u,", totllr);
                for (size_t z = 0; z < N_DETZONE; z++) {
                    fprintf(fp, "%u,", DETGETDAY_(llr));
                }
            }
        }
        fprintf(fp, "\n");
    }
}

void* _windowzonedetection_consumer(void* argsvoid) {
    struct td_consumer_args* args = argsvoid;

    const size_t N_CONFIG = configsuite.configs.number;

    while(!_wzonequeue_isend(args->queue)) {
        char path[PATH_MAX];
        _wzonequeue_data* qm;
        FILE* file;
        Performance thchooser_fpr;
        RWindowMC windowmc;

        qm = _wzonequeue_dequeue(args->queue);
        
        if (qm == NULL) break;

        windowmc = qm->windowmc;
        
        LOG_TRACE("consumer: processing windowzone %ld.", windowmc->g2index);

        {
            Detection* detection;
            DetectionZone* zones[2];
            
            detection = detection_alloc();

            const size_t idxconfig = qm->idxconfig;

            memset(detection, 0, sizeof(Detection));

            _windowzone_detect(windowmc, idxconfig, detection);
        } // filling Result

        free(qm);
    }

    return NULL;
}

void* windowzonedetection_start() {
    char outdir[PATH_MAX];

    windowzonedetection_context* context = calloc(1, sizeof(windowzonedetection_context));

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
    context->qm = calloc(buffer_size, sizeof(_wzonequeue_data));

    {
        _wzonequeue_t queue = TDQUEUE_INITIALIZER(context->qm, buffer_size);
        memcpy(&context->queue, &queue, sizeof(_wzonequeue_t));
    }

    context->producer_args.queue = &context->queue;

    pthread_create(&context->producer, NULL, _windowzonedetection_producer, &context->producer_args);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        context->consumer_args[i].id = i;
        context->consumer_args[i].queue = &context->queue;

        pthread_create(&context->consumers[i], NULL, _windowzonedetection_consumer, &context->consumer_args[i]);
    }

    return context;
}

void windowzonedetection_wait(void* context_void) {
    assert(context_void);

    windowzonedetection_context* context = context_void;

    pthread_join(context->producer, NULL);

    for (size_t i = 0; i < TD_NTHREADS; ++ i) {
        pthread_join(context->consumers[i], NULL);
    }

    free(context->qm);
    free(context);
}


void _windowzonedetection_io_detection(IOReadWrite rw, FILE* file, Detection* detection) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRWSIZE(detection, sizeof(Detection));
}

inline void _windowzonedetection_io_path(char path[PATH_MAX], RWindowMC windowmc) {
    sprintf(path, "/trainer/result_windowmc_%ld", windowmc->g2index);
    io_path_concat(windowing_iodir, path, path);
}