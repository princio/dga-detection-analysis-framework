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
            INITMANY(results->wsize._[w].apply._[a].thchooser, thchoosers.number, ResultsThChooser);
            for (size_t t = 0; t < thchoosers.number; t++) {
                INITMANY(results->wsize._[w].apply._[a].thchooser._[t].kfold, kfold0->config.kfolds, Result);
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

                        Detection* detection = &results->wsize._[i].apply._[a].thchooser._[ev].kfold._[k].best_test;

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
                        Result* result = &RESULT_IDX((*results), i, a, ev, k);

                        result->threshold_chooser = &thchoosers._[ev];

                        memcpy(&results->wsize._[i].apply._[a].thchooser._[ev].kfold._[k].best_train, &best_detections[a]._[ev], sizeof(Detection));
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
            for (size_t t = 0; t < results->wsize._[w].apply._[a].thchooser.number; t++) {
                FREEMANY(results->wsize._[w].apply._[a].thchooser._[t].kfold);
            }
            FREEMANY(results->wsize._[w].apply._[a].thchooser);
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
            for (size_t t = 0; t < results->wsize._[i].apply._[a].thchooser.number; t++) {
                for (size_t k = 0; k < results->wsize._[i].apply._[a].thchooser._[t].kfold.number; k++) {
                    Result* result = &results->wsize._[i].apply._[a].thchooser._[t].kfold._[k];

                    trainer_io_detection(rw, file, &result->best_train);
                    trainer_io_detection(rw, file, &result->best_test);

                    if (IO_READ) {
                        result->threshold_chooser = &trainer->thchoosers._[t];
                    }
                }
            }
        }
    }
}

void trainer_io(IOReadWrite rw, char dirname[200], RKFold0 kfold0, RTrainer* results) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    char fpath[210];
    FILE* file;
    MANY(Performance) thchoosers;

    if (rw == IO_WRITE) {
        CLONEMANY(thchoosers, (*results)->thchoosers, Performance);
    }

    sprintf(fpath, "%s/trainer.bin", dirname);

    file = io_openfile(rw, fpath);

    trainer_io_thchoosers(rw, file, &thchoosers);

    if (rw == IO_READ) {
        *results = trainer_create(kfold0, thchoosers);
    }

    FREEMANY(thchoosers);

    trainer_io_results(rw, file, *results);

    fclose(file);

    if (rw == IO_WRITE) {
        printf("Results saved in: <%s>\n", fpath);
    }
}