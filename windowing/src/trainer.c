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

MANY(ThsDataset) _trainer_ths(RDataset0 dataset, const size_t applies_number) {
    MANY(ThsDataset) ths;

    INITMANY(ths, applies_number, MANY(ThsDataset));

    for (size_t a = 0; a < applies_number; a++) {
        INITMANY(ths._[a], dataset->windows.all.number, ThsDataset);
    }

    for (size_t a = 0; a < applies_number; a++) {
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

RTrainer trainer_create(RTestBed2 tb2, MANY(Performance) thchoosers) {
    RTrainer trainer;

    trainer = calloc(1, sizeof(__Trainer));

    TrainerResults* results = &trainer->results;

    trainer->tb2 = tb2;
    trainer->thchoosers = thchoosers;

    CLONEMANY(trainer->thchoosers, thchoosers);
    
    INITMANY(results->bywsize, tb2->wsizes.number, ResultsWSize);
    for (size_t w = 0; w < tb2->wsizes.number; w++) {
        INITMANY(results->bywsize._[w].byapply, tb2->applies.number, ResultsApply);
        for (size_t a = 0; a < tb2->applies.number; a++) {
            INITMANY(results->bywsize._[w].byapply._[a].byfold, tb2->folds.number, ResultsFold);
            for (size_t f = 0; f < tb2->folds.number; f++) {
                INITMANY(results->bywsize._[w].byapply._[a].byfold._[f].bytry, tb2->folds._[f]->config.tries, ResultsFoldTries);
                for (size_t try = 0; try < tb2->folds._[f]->config.tries; try++) {
                    INITMANY(results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits, tb2->folds._[f]->config.k, ResultsFoldTriesSplits);
                    for (size_t k = 0; k < tb2->folds._[f]->config.k; k++) {
                        INITMANY(results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits._[k].bythchooser, thchoosers.number, Result);
                    }
                }
            }
        }
    }

    return trainer;
}

RTrainer trainer_run(RTestBed2 tb2, MANY(Performance) thchoosers) {
    RTrainer trainer = trainer_create(tb2, thchoosers);
    TrainerResults* results = &trainer->results;

    testbed2_apply(tb2);

    int r = 0;
    for (size_t idxfold = 0; idxfold < tb2->folds.number; idxfold++) {
        for (size_t try = 0; try < tb2->folds._[idxfold]->config.tries; try++) {
            for (size_t idxwsize = 0; idxwsize < tb2->wsizes.number; idxwsize++) {
                DatasetSplits const * const splits = &tb2->folds._[idxfold]->tries._[try].bywsize._[idxwsize].splits;

                if (!splits->isok) {
                    printf("Warning: skipping folding for fold=%ld, try=%ld, wsize=%ld\n", idxfold, try, tb2->wsizes._[idxwsize].value);
                    continue;
                }

                for (size_t idxsplit = 0; idxsplit < splits->splits.number; idxsplit++) {
                    DatasetSplit0 split = splits->splits._[idxsplit];

                    MANY(ThsDataset) ths = _trainer_ths(split.train, tb2->applies.number);
                    MANY(Detection) detections[tb2->applies.number];

                    for (size_t a = 0; a < tb2->applies.number; a++) {
                        INITMANY(detections[a], ths._[a].number, Detection);
                    }

                    for (size_t w = 0; w < split.train->windows.all.number; w++) {
                        RWindow0 window0 = split.train->windows.all._[w];
                        RSource source = window0->windowing->source;

                        for (size_t a = 0; a < tb2->applies.number; a++) {
                            for (size_t ev = 0; ev < ths._[a].number; ev++) {
                                double th = ths._[a]._[ev];
                                const int prediction = window0->applies._[a].logit >= th;
                                const int infected = window0->windowing->source->wclass.mc > 0;
                                const int is_true = prediction == infected;
                                detections[a]._[ev].th = th;
                                detections[a]._[ev].windows[source->wclass.mc][is_true]++;
                                detections[a]._[ev].sources[source->wclass.mc][source->index.multi][is_true]++;
                            }
                        }
                    }
                
                    MANY(Detection) best_detections[tb2->applies.number];
                    int best_detections_init[tb2->applies.number][thchoosers.number];
                    memset(best_detections_init, 0, sizeof(int) * tb2->applies.number * thchoosers.number);

                    for (size_t a = 0; a < tb2->applies.number; a++) {
                        INITMANY(best_detections[a], thchoosers.number, Detection);
                    }

                    for (size_t idxapply = 0; idxapply < tb2->applies.number; idxapply++) {
                        for (size_t idxth = 0; idxth < ths._[idxapply].number; idxth++) {
                            for (size_t idxthchooser = 0; idxthchooser < thchoosers.number; idxthchooser++) {
                                double current_score = detect_performance(&detections[idxapply]._[idxth], &thchoosers._[idxthchooser]);

                                int is_better = 0;
                                if (best_detections_init[idxapply][idxthchooser] == 1) {
                                    double best_score = detect_performance(&best_detections[idxapply]._[idxthchooser], &thchoosers._[idxthchooser]);
                                    is_better = detect_performance_compare(&thchoosers._[idxthchooser], current_score, best_score);
                                } else {
                                    is_better = 1;
                                }

                                if (is_better) {
                                    best_detections_init[idxapply][idxthchooser] = 1;
                                    memcpy(&best_detections[idxapply]._[idxthchooser], &detections[idxapply]._[idxth], sizeof(Detection));
                                }
                            }
                        }
                    }

                    for (size_t w = 0; w < split.test->windows.all.number; w++) {
                        RWindow0 window0 = split.test->windows.all._[w];
                        RSource source = window0->windowing->source;

                        for (size_t a = 0; a < tb2->applies.number; a++) {
                            for (size_t ev = 0; ev < thchoosers.number; ev++) {
                                double th = best_detections[a]._[ev].th;

                                Detection* detection = &results->bywsize._[idxwsize].byapply._[a].byfold._[idxfold].bytry._[try].bysplits._[idxsplit].bythchooser._[ev].best_test;

                                const int prediction = window0->applies._[a].logit >= th;
                                const int infected = window0->windowing->source->wclass.mc > 0;
                                const int is_true = prediction == infected;

                                detection->th = th;
                                detection->windows[source->wclass.mc][is_true]++;
                                detection->sources[source->wclass.mc][source->index.multi][is_true]++;
                            }
                        }
                        
                        for (size_t a = 0; a < tb2->applies.number; a++) {
                            for (size_t ev = 0; ev < thchoosers.number; ev++) {
                                Result* result = &RESULT_IDX((*results), idxwsize, a, idxfold, try, idxsplit, ev);

                                result->threshold_chooser = &thchoosers._[ev];

                                memcpy(&results->bywsize._[idxwsize].byapply._[a].byfold._[idxfold].bytry._[try].bysplits._[idxsplit].bythchooser._[ev].best_train, &best_detections[a]._[ev], sizeof(Detection));
                            }
                        }
                    }

                    for (size_t a = 0; a < tb2->applies.number; a++) {
                        FREEMANY(best_detections[a]);
                    }

                    for (size_t a = 0; a < tb2->applies.number; a++) {
                        FREEMANY(detections[a]);
                    }

                    _trainer_ths_free(ths);
                }
            }
        }
    }

    return trainer;
}

void trainer_free(RTrainer trainer) {
    TrainerResults* results = &trainer->results;
    for (size_t w = 0; w < results->bywsize.number; w++) {
        for (size_t a = 0; a < results->bywsize._[w].byapply.number; a++) {
            for (size_t f = 0; f < results->bywsize._[w].byapply._[a].byfold.number; f++) {
                for (size_t try = 0; try < results->bywsize._[w].byapply._[a].byfold._[f].bytry.number; try++) {
                    for (size_t k = 0; k < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits.number; k++) {
                        FREEMANY(results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits._[k].bythchooser);
                    }
                    FREEMANY(results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits);
                }
                FREEMANY(results->bywsize._[w].byapply._[a].byfold._[f].bytry);
            }
            FREEMANY(results->bywsize._[w].byapply._[a].byfold);
        }
        FREEMANY(results->bywsize._[w].byapply);
    }
    FREEMANY(results->bywsize);
    FREEMANY(trainer->thchoosers);
    free(trainer);
}

void trainer_io_thchoosers(IOReadWrite rw, FILE* file, MANY(Performance)* thchoosers) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(thchoosers->number);

    if (rw == IO_READ) {
        INITMANYREF(thchoosers, thchoosers->number, Performance);
    }

    for (size_t ev = 0; ev < thchoosers->number; ev++) {
        FRW(thchoosers->_[ev].dgadet);
        FRW(thchoosers->_[ev].name);
        FRW(thchoosers->_[ev].greater_is_better);
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

    for (size_t w = 0; w < results->bywsize.number; w++) {
        for (size_t a = 0; a < results->bywsize._[w].byapply.number; a++) {
            for (size_t f = 0; f < results->bywsize._[w].byapply._[a].byfold.number; f++) {
                for (size_t try = 0; try < results->bywsize._[w].byapply._[a].byfold._[f].bytry.number; try++) {
                    for (size_t k = 0; k < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits.number; k++) {
                        for (size_t ev = 0; ev < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits._[k].bythchooser.number; ev++) {
                            Result* result = &RESULT_IDX((*results), w, a, f, try, k, ev);

                            trainer_io_detection(rw, file, &result->best_train);
                            trainer_io_detection(rw, file, &result->best_test);

                            if (IO_READ) {
                                result->threshold_chooser = &trainer->thchoosers._[ev];
                            }
                        }
                    }
                }
            }
        }
    }
}

void trainer_io(IOReadWrite rw, char dirname[200], RTestBed2 tb2, RTrainer* trainer) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    char fpath[210];
    FILE* file;
    MANY(Performance) thchoosers;

    if (rw == IO_WRITE) {
        CLONEMANY(thchoosers, (*trainer)->thchoosers);
    }

    sprintf(fpath, "%s/trainer.bin", dirname);

    file = io_openfile(rw, fpath);

    trainer_io_thchoosers(rw, file, &thchoosers);

    if (rw == IO_READ) {
        *trainer = trainer_create(tb2, thchoosers);
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
    TrainerResults* results;
    RTestBed2 tb2;
    RFold fold;
    FILE* file;

    sprintf(fpath, "%s/trainer.md", dirname);
    file = fopen(fpath, "w");

    results = &trainer->results;
    tb2 = trainer->tb2;

    #define FP(...) fprintf(file, __VA_ARGS__);
    #define FPNL(N, ...) fprintf(file, __VA_ARGS__); for (size_t i = 0; i < N; i++) fprintf(file, "\n");
    #define TR(CM, CL) ((double) (CM)[CL][1]) / ((CM)[CL][0] + (CM)[CL][1])

    FPNL(2, "# Trainer");

    {
        time_t time_now;
        struct tm tm;
        time_now = time(NULL);
        tm = *localtime(&time_now);
        FPNL(3, "Saved on date: %d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

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

        for (size_t w = 0; w < results->bywsize.number; w++) {
            for (size_t a = 0; a < results->bywsize._[w].byapply.number; a++) {
                for (size_t f = 0; f < results->bywsize._[w].byapply._[a].byfold.number; f++) {
                    for (size_t try = 0; try < results->bywsize._[w].byapply._[a].byfold._[f].bytry.number; try++) {
                        for (size_t k = 0; k < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits.number; k++) {
                            for (size_t ev = 0; ev < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits._[k].bythchooser.number; ev++) {
                                Result* result = &RESULT_IDX((*results), w, a, f, try, k, ev);

                                FP("|%*ld",  len_header, trainer->tb2->wsizes._[w].value);
                                FP("|%*ld",  len_header, a);
                                FP("|%*ld",  len_header, k);
                                FP("|%*s",  len_header, trainer->thchoosers._[ev].name);
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
        }
    }

    #undef FP
    #undef FPNL
    #undef TP

    fclose(file);
}