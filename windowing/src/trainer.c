#include "trainer.h"

#include "dataset.h"
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

Results trainer_run(TestBed2* tb2, MANY(Performance) thchoosers, KFoldConfig0 config) {
    Results results;

    INITMANY(results.wsize, tb2->wsizes.number, ResultsWSize);
    for (size_t w = 0; w < tb2->wsizes.number; w++) {
        INITMANY(results.wsize._[w].apply, tb2->psets.number, ResultsApply);
        for (size_t a = 0; a < tb2->psets.number; a++) {
            INITMANY(results.wsize._[w].apply._[a].thchooser, thchoosers.number, ResultsThChooser);
            for (size_t t = 0; t < thchoosers.number; t++) {
                INITMANY(results.wsize._[w].apply._[a].thchooser._[t].kfold, config.kfolds, Result);
            }
        }
    }

    INITMANY(results.kfolds, tb2->datasets.wsize.number, KFold0);

    int r = 0;
    for (size_t i = 0; i < tb2->datasets.wsize.number; i++) {
        KFold0 kfold = kfold0_run(tb2->datasets.wsize._[i], config);

        results.kfolds._[i] = kfold;

        if (!kfold0_ok(&kfold)) {
            printf("Warning: skipping folding for wsize=%u\n", tb2->wsizes._[i]);
            continue;
        }

        for (size_t k = 0; k < kfold.splits.number; k++) {
            DatasetSplit0 split = kfold.splits._[k];

            MANY(ThsDataset) ths = _trainer_ths(split.train);
            MANY(Detection) detections[tb2->psets.number];

            for (size_t a = 0; a < tb2->psets.number; a++) {
                INITMANY(detections[a], ths._[a].number, Detection);
            }

            for (size_t w = 0; w < split.train->windows.all.number; w++) {
                RWindow0 window0 = split.train->windows.all._[w];
                RSource source = window0->windowing->source;

                for (size_t a = 0; a < tb2->psets.number; a++) {
                    for (size_t t = 0; t < ths._[a].number; t++) {
                        double th = ths._[a]._[t];
                        const int prediction = window0->applies._[a].logit >= th;
                        const int infected = window0->windowing->source->wclass.mc > 0;
                        const int is_true = prediction == infected;
                        detections[a]._[t].th = th;
                        detections[a]._[t].windows[source->wclass.mc][is_true]++;
                        detections[a]._[t].sources[source->wclass.mc][source->index][is_true]++;
                    }
                }
            }
        
            MANY(Detection) best_detections[tb2->psets.number];
            int best_detections_init[tb2->psets.number][thchoosers.number];
            memset(best_detections_init, 0, sizeof(int) * tb2->psets.number * thchoosers.number);

            for (size_t a = 0; a < tb2->psets.number; a++) {
                INITMANY(best_detections[a], thchoosers.number, Detection);
            }

            for (size_t a = 0; a < tb2->psets.number; a++) {
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

                for (size_t a = 0; a < tb2->psets.number; a++) {
                    for (size_t ev = 0; ev < thchoosers.number; ev++) {
                        double th = best_detections[a]._[ev].th;

                        Detection* detection = &results.wsize._[i].apply._[a].thchooser._[ev].kfold._[k].best_test;

                        const int prediction = window0->applies._[a].logit >= th;
                        const int infected = window0->windowing->source->wclass.mc > 0;
                        const int is_true = prediction == infected;

                        detection->th = th;
                        detection->windows[source->wclass.mc][is_true]++;
                        detection->sources[source->wclass.mc][source->index][is_true]++;
                    }
                }
                
                for (size_t a = 0; a < tb2->psets.number; a++) {
                    for (size_t ev = 0; ev < thchoosers.number; ev++) {
                        Result* result = &RESULT_IDX(results, i, a, ev, k);

                        result->threshold_chooser = &thchoosers._[ev];

                        memcpy(&results.wsize._[i].apply._[a].thchooser._[ev].kfold._[k].best_train, &best_detections[a]._[ev], sizeof(Detection));
                    }
                }
            }

            for (size_t a = 0; a < tb2->psets.number; a++) {
                FREEMANY(best_detections[a]);
            }

            for (size_t a = 0; a < tb2->psets.number; a++) {
                FREEMANY(detections[a]);
            }

            _trainer_ths_free(ths);
        }
    }

    return results;
}

void trainer_free(Results* results) {
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
    for (size_t i = 0; i < results->kfolds.number; i++) {
        kfold0_free(&results->kfolds._[i]);
    }
    FREEMANY(results->kfolds);
}