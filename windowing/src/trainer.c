#include "trainer.h"

#include "dataset.h"
#include "io.h"
#include "parameters.h"
#include "detect.h"
#include "sources.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void trainer_md(char dirname[200], RTrainer trainer);

typedef MANY(double) ThsDataset;
typedef MANY(Detection) DetectionDataset[3];
MAKEMANY(ThsDataset);

MANY(ThsDataset) _trainer_ths(RDataset0 dataset) {
    MANY(ThsDataset) ths;

    INITMANY(ths, dataset->applies_number, MANY(ThsDataset));

    for (size_t a = 0; a < dataset->applies_number; a++) {
        INITMANY(ths._[a], dataset->windows.all.number, ThsDataset);
    }

    for (size_t a = 0; a < dataset->applies_number; a++) {
        size_t n = 0;

        for (size_t w = 0; w < dataset->windows.all.number; w++) {
            int logit;
            int exists;

            logit = floor(dataset->windows.all._[w]->applies._[a].logit);
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
    }

    return ths;
}

void _trainer_ths_free(MANY(ThsDataset) ths) {
    for (size_t i = 0; i < ths.number; i++) {
        FREEMANY(ths._[i]);
    }
    FREEMANY(ths);
}

RTrainer trainer_create(RKFold0 kfold0, MANY(Performance) thchoosers) {
    RTrainer trainer;

    trainer = calloc(1, sizeof(__Trainer));

    TrainerResults* results = &trainer->results;

    trainer->kfold0 = kfold0;
    trainer->thchoosers = thchoosers;

    CLONEMANY(trainer->thchoosers, thchoosers, Performance);
    
    INITMANY(results->wsize, kfold0->testbed2->wsizes.number, ResultsWSize);
    for (size_t w = 0; w < kfold0->testbed2->wsizes.number; w++) {
        INITMANY(results->wsize._[w].apply, kfold0->testbed2->psets.number, ResultsApply);
        for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
            INITMANY(results->wsize._[w].apply._[a].fold_splits, kfold0->config.kfolds, ResultsSplits);
            for (size_t k = 0; k < kfold0->config.kfolds; k++) {
                INITMANY(results->wsize._[w].apply._[a].fold_splits._[k].thchooser, thchoosers.number, Result);
            }
        }
    }

    return trainer;
}

RTrainer trainer_run(RKFold0 kfold0, MANY(Performance) thchoosers) {
    RTrainer trainer = trainer_create(kfold0, thchoosers);
    TrainerResults* results = &trainer->results;

    int r = 0;
    for (size_t i = 0; i < kfold0->datasets.wsize.number; i++) {
        MANY(DatasetSplit0) ds_splits = kfold0->datasets.wsize._[i].splits;

        if (!dataset0_splits_ok(ds_splits)) {
            printf("Warning: skipping folding for wsize=%ld\n", kfold0->testbed2->wsizes._[i].value);
            continue;
        }

        for (size_t k = 0; k < ds_splits.number; k++) {
            DatasetSplit0 split = ds_splits._[k];

            MANY(ThsDataset) ths = _trainer_ths(split.train);
            MANY(Detection) detections[kfold0->testbed2->psets.number];

            for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                INITMANY(detections[a], ths._[a].number, Detection);
            }

            for (size_t w = 0; w < split.train->windows.all.number; w++) {
                RWindow0 window0 = split.train->windows.all._[w];
                RSource source = window0->windowing->source;

                for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                    for (size_t t = 0; t < ths._[a].number; t++) {
                        double th = ths._[a]._[t];
                        const int prediction = window0->applies._[a].logit >= th;
                        const int infected = window0->windowing->source->wclass.mc > 0;
                        const int is_true = prediction == infected;
                        detections[a]._[t].th = th;
                        detections[a]._[t].windows[source->wclass.mc][is_true]++;
                        detections[a]._[t].sources[source->wclass.mc][source->index.multi][is_true]++;
                    }
                }
            }
        
            MANY(Detection) best_detections[kfold0->testbed2->psets.number];
            int best_detections_init[kfold0->testbed2->psets.number][thchoosers.number];
            memset(best_detections_init, 0, sizeof(int) * kfold0->testbed2->psets.number * thchoosers.number);

            for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                INITMANY(best_detections[a], thchoosers.number, Detection);
            }

            for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                for (size_t t = 0; t < ths._[a].number; t++) {
                    for (size_t ev = 0; ev < thchoosers.number; ev++) {
                        double current_score = detect_performance(&detections[a]._[t], &thchoosers._[ev]);

                        int is_better = 0;
                        if (best_detections_init[a][ev] == 1) {
                            double best_score = detect_performance(&best_detections[a]._[ev], &thchoosers._[ev]);
                            is_better = detect_performance_compare(&thchoosers._[ev], current_score, best_score);
                        } else {
                            is_better = 1;
                        }

                        if (is_better) {
                            best_detections_init[a][ev] = 1;
                            memcpy(&best_detections[a]._[ev], &detections[a]._[t], sizeof(Detection));
                        }
                    }
                }
            }

            for (size_t w = 0; w < split.test->windows.all.number; w++) {
                RWindow0 window0 = split.test->windows.all._[w];
                RSource source = window0->windowing->source;

                for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                    for (size_t ev = 0; ev < thchoosers.number; ev++) {
                        double th = best_detections[a]._[ev].th;

                        Detection* detection = &results->wsize._[i].apply._[a].fold_splits._[k].thchooser._[ev].best_test;

                        const int prediction = window0->applies._[a].logit >= th;
                        const int infected = window0->windowing->source->wclass.mc > 0;
                        const int is_true = prediction == infected;

                        detection->th = th;
                        detection->windows[source->wclass.mc][is_true]++;
                        detection->sources[source->wclass.mc][source->index.multi][is_true]++;
                    }
                }
                
                for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                    for (size_t ev = 0; ev < thchoosers.number; ev++) {
                        Result* result = &RESULT_IDX((*results), i, a, k, ev);

                        result->threshold_chooser = &thchoosers._[ev];

                        memcpy(&results->wsize._[i].apply._[a].fold_splits._[k].thchooser._[ev].best_train, &best_detections[a]._[ev], sizeof(Detection));
                    }
                }
            }

            for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                FREEMANY(best_detections[a]);
            }

            for (size_t a = 0; a < kfold0->testbed2->psets.number; a++) {
                FREEMANY(detections[a]);
            }

            _trainer_ths_free(ths);
        }
    }

    return trainer;
}

void trainer_free(RTrainer trainer) {
    TrainerResults* results = &trainer->results;
    for (size_t w = 0; w < results->wsize.number; w++) {
        for (size_t a = 0; a < results->wsize._[w].apply.number; a++) {
            for (size_t s = 0; s < results->wsize._[w].apply._[a].fold_splits.number; s++) {
                FREEMANY(results->wsize._[w].apply._[a].fold_splits._[s].thchooser);
            }
            FREEMANY(results->wsize._[w].apply._[a].fold_splits);
        }
        FREEMANY(results->wsize._[w].apply);
    }
    FREEMANY(results->wsize);
    FREEMANY(trainer->thchoosers);
    free(trainer);
}

void trainer_io_thchoosers(IOReadWrite rw, FILE* file, MANY(Performance)* thchoosers) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(thchoosers->number);

    if (rw == IO_READ) {
        INITMANYREF(thchoosers, thchoosers->number, Performance);
    }

    for (size_t t = 0; t < thchoosers->number; t++) {
        FRW(thchoosers->_[t].dgadet);
        FRW(thchoosers->_[t].name);
        FRW(thchoosers->_[t].greater_is_better);
    }
}

void trainer_io_detection(IOReadWrite rw, FILE* file, Detection* detection) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(detection->th);
    FRW(detection->sources);
    FRW(detection->windows);
}

void trainer_io_results(IOReadWrite rw, FILE* file, RTrainer trainer) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    TrainerResults* results = &trainer->results;

    for (size_t i = 0; i < results->wsize.number; i++) {
        for (size_t a = 0; a < results->wsize._[i].apply.number; a++) {
            for (size_t s = 0; s < results->wsize._[i].apply._[a].fold_splits.number; s++) {
                for (size_t t = 0; t < results->wsize._[i].apply._[a].fold_splits._[s].thchooser.number; t++) {
                    Result* result = &results->wsize._[i].apply._[a].fold_splits._[s].thchooser._[t];

                    trainer_io_detection(rw, file, &result->best_train);
                    trainer_io_detection(rw, file, &result->best_test);

                    if (IO_READ) {
                        result->threshold_chooser = &trainer->thchoosers._[s];
                    }
                }
            }
        }
    }
}

void trainer_io(IOReadWrite rw, char dirname[200], RKFold0 kfold0, RTrainer* trainer) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    char fpath[210];
    FILE* file;
    MANY(Performance) thchoosers;

    if (rw == IO_WRITE) {
        CLONEMANY(thchoosers, (*trainer)->thchoosers, Performance);
    }

    sprintf(fpath, "%s/trainer.bin", dirname);

    file = io_openfile(rw, fpath);

    trainer_io_thchoosers(rw, file, &thchoosers);

    if (rw == IO_READ) {
        *trainer = trainer_create(kfold0, thchoosers);
    }

    FREEMANY(thchoosers);

    trainer_io_results(rw, file, *trainer);

    fclose(file);

    if (rw == IO_WRITE) {
        printf("Results saved in: <%s>\n", fpath);
        trainer_md(dirname, *trainer);
    }
}

void trainer_md(char dirname[200], RTrainer trainer) {
    char fpath[210];
    time_t t;
    struct tm tm;
    TrainerResults* results;
    RTestBed2 tb2;
    RKFold0 kfold0;
    FILE* file;

    sprintf(fpath, "%s/trainer.md", dirname);
    file = fopen(fpath, "w");

    t = time(NULL);
    tm = *localtime(&t);

    results = &trainer->results;
    tb2 = trainer->kfold0->testbed2;
    kfold0 = trainer->kfold0;

    #define FP(...) fprintf(file, __VA_ARGS__);
    #define FPNL(N, ...) fprintf(file, __VA_ARGS__); for (size_t i = 0; i < N; i++) fprintf(file, "\n");
    #define TR(CM, CL) ((double) (CM)[CL][1]) / ((CM)[CL][0] + (CM)[CL][1])

    FPNL(2, "# Trainer");

    FPNL(3, "Saved on date: %d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    FPNL(2, "## Results");

    {
        const int len_header = 15;
        const char* headers[] = {
            "wsize",
            "pset",
            "k",
            "thchooser",
            "train TR(0)",
            "train TR(1)",
            "train TR(2)",
            "test TR(0)",
            "test TR(1)",
            "test TR(2)"
        };
        const size_t n_headers = sizeof (headers) / sizeof (const char *);
        for (size_t i = 0; i < n_headers; i++) {
            FP("|%*s", len_header, headers[i]);
        }
        FPNL(1, "|");
        for (size_t i = 0; i < n_headers; i++) {
            FP("|");
            for (int t = 0; t < len_header; t++) {
                FP("-");
            }
        }
        FPNL(1, "|");

        for (size_t w = 0; w < tb2->wsizes.number; w++) {
            for (size_t a = 0; a < tb2->psets.number; a++) {
                for (size_t k = 0; k < trainer->kfold0->config.kfolds; k++) {
                    for (size_t t = 0; t < trainer->thchoosers.number; t++) {
                        Result* result = &RESULT_IDX((*results), w, a, k, t);

                        FP("|%*ld",  len_header, trainer->kfold0->testbed2->wsizes._[w].value);
                        FP("|%*ld",  len_header, a);
                        FP("|%*ld",  len_header, k);
                        FP("|%*s",  len_header, trainer->thchoosers._[t].name);
                        DGAFOR(cl) {
                            FP("|%*.4f",  len_header, TR(result->best_train.windows, cl));
                        }
                        DGAFOR(cl) {
                            FP("|%*.4f",  len_header, TR(result->best_test.windows, cl));
                        }
                        FPNL(1, "|");
                    }
                }
            }
        }
    }

    #undef FP
    #undef FPNL
    #undef TP
}