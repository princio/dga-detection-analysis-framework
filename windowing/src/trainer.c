#include "trainer.h"

#include "dataset.h"
#include "io.h"
#include "logger.h"
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

#define APPLY_SKIP if (results_todo._[idxapply].apply == 0) { continue; }
#define THCHOOSER_SKIP if (results_todo._[idxapply].thchoosers._[idxthchooser] == 0) { continue; }

typedef int TODO;
MAKEMANY(TODO);

typedef struct ResultsTODO {
    int apply;
    MANY(TODO) thchoosers;
} ResultsTODO;

MAKEMANY(ResultsTODO);

void trainer_md(char dirname[200], RTrainer trainer);
int trainer_io_results_file(IOReadWrite rw, char fpath[PATH_MAX], TrainerBy_splits* result);

typedef MANY(double) ThsDataset;
typedef MANY(Detection) DetectionDataset[3];
MAKEMANY(ThsDataset);

MANY(ThsDataset) _trainer_ths(RDataset0 dataset, MANY(ResultsTODO) todo) {
    MANY(ThsDataset) ths;

    INITMANY(ths, todo.number, MANY(ThsDataset));

    for (size_t a = 0; a < todo.number; a++) {
        if (todo._[a].apply) {
            INITMANY(ths._[a], dataset->windows.all.number, ThsDataset);
        }
    }

    size_t min = SIZE_MAX;
    size_t max = - SIZE_MAX;
    size_t avg = 0;
    int th_reducer;

    th_reducer = 50;
    // if (dataset->windows.all.number < 10000) th_reducer = 25;
    // else
    // if (dataset->windows.all.number < 1000) th_reducer = 10;
    // else
    // if (dataset->windows.all.number < 100) th_reducer = 5;
    
    for (size_t a = 0; a < todo.number; a++) {
        if (todo._[a].apply == 0) {
            continue;
        }
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
    
    LOG_TRACE("Ths number: reducer=%d\twn=%-10ld\tmin=%-10ld\tmax=%-10ld\tavg=%-10ld", th_reducer, dataset->windows.all.number, min, max, avg / todo.number);

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

    TrainerBy* by = &trainer->by;

    trainer->tb2 = tb2;
    trainer->thchoosers = thchoosers;

    CLONEMANY(trainer->thchoosers, thchoosers);

    INITBY_N((*by), wsize, tb2->wsizes.number);
    INITBY_N((*by), apply, tb2->applies.number);
    INITBY_N((*by), fold, tb2->dataset.n.fold);
    INITBY_N((*by), try, tb2->dataset.n.try);
    INITBY_N((*by), thchooser, thchoosers.number);

    INITBYFOR((*by), (*by), wsize, TrainerBy) {
        INITBYFOR((*by), GETBY((*by), wsize), apply, TrainerBy) {
            INITBYFOR((*by), GETBY2((*by), wsize, apply), fold, TrainerBy) {
                INITBYFOR((*by), GETBY3((*by), wsize, apply, fold), try, TrainerBy) {
                    INITBYFOR((*by), GETBY4((*by), wsize, apply, fold, try), thchooser, TrainerBy) {
                        INITMANY(GETBY5((*by), wsize, apply, fold, try, thchooser).splits, tb2->dataset.folds._[idxfold].k, TrainerBy_splits);
                    }
                }
            }
        }
    }

    return trainer;
}

RTrainer trainer_run(RTestBed2 tb2, MANY(Performance) thchoosers, char rootdir[PATH_MAX - 100]) {
    RTrainer trainer = trainer_create(tb2, thchoosers);
    TrainerBy* by = &trainer->by;

    testbed2_apply(tb2);

    printf("%-7s | ", "fold");
    printf("%-7s | ", "try");
    printf("%-7s | ", "wsize");
    printf("%-7s | ", "splits");
    printf("%-7s\n", "skipped");
    int r = 0;
    FORBY((*by), fold) {
        FORBY((*by), try) {
            FORBY((*by), wsize) {
                TCPC(DatasetSplits) splits = &GETBY3(tb2->dataset, wsize, try, fold);
                if (!splits->isok) {
                    LOG_WARN("Warning: skipping folding for fold=%ld, try=%ld, wsize=%ld\n", idxfold, idxtry, tb2->wsizes._[idxwsize].value);
                    continue;
                }

                for (size_t k = 0; k < splits->splits.number; k++) {
                    printf("%3ld/%-3ld | ", 1+idxfold, tb2->dataset.n.fold);
                    printf("%3ld/%-3ld | ", 1+idxtry, tb2->dataset.n.try);
                    printf("%3ld/%-3ld | ", 1+idxwsize, tb2->wsizes.number);
                    printf("%3ld/%-3ld | ", 1+k, splits->splits.number);

                    DatasetSplit0 split = splits->splits._[k];
                    MANY(ResultsTODO) results_todo;
                    MANY(ThsDataset) ths;
                    MANY(Detection) detections[by->n.apply];
                    MANY(Detection) best_detections[by->n.apply];
                    int best_detections_init[by->n.apply][thchoosers.number];
                    size_t skipped[2];

                    {
                        char fpath[PATH_MAX];
                        INITMANY(results_todo, by->n.apply, ResultsTODO);
                        skipped[0] = 0;
                        skipped[1] = 0;
                        FORBY((*by), apply) {
                            INITMANY(results_todo._[idxapply].thchoosers, thchoosers.number, TODO);
                            FORBY((*by), thchooser) {
                                sprintf(fpath, "results_%ld_%ld_%ld_%ld_%s__%ld.bin", idxfold, idxtry, idxwsize, idxapply, thchoosers._[idxthchooser].name, k);
                                io_path_concat(rootdir, fpath, fpath);
                                if (trainer_io_results_file(IO_READ, fpath, &GETBY5((*by), wsize, apply, fold, try, thchooser).splits._[k])) {
                                    results_todo._[idxapply].thchoosers._[idxthchooser] = 1;
                                    results_todo._[idxapply].apply = 1;
                                } else {
                                    skipped[0]++;
                                }
                                skipped[1]++;
                            }
                            if (results_todo._[idxapply].apply == 0) {
                                FREEMANY(results_todo._[idxapply].thchoosers);
                            }
                        }
                    }

                    printf("%3ld/%-3ld\n", skipped[0], skipped[1]);

                    memset(best_detections_init, 0, sizeof(int) * by->n.apply * thchoosers.number);
                    ths = _trainer_ths(split.train, results_todo);

                    FORBY((*by), apply) { APPLY_SKIP;
                        INITMANY(detections[idxapply], ths._[idxapply].number, Detection);
                        INITMANY(best_detections[idxapply], thchoosers.number, Detection);
                    }

                    for (size_t idxwindow = 0; idxwindow < split.train->windows.all.number; idxwindow++) {
                        RWindow0 window0 = split.train->windows.all._[idxwindow];
                        RSource source = window0->windowing->source;


                        FORBY((*by), apply) { APPLY_SKIP;
                            for (size_t idxth = 0; idxth < ths._[idxapply].number; idxth++) {
                                double th = ths._[idxapply]._[idxth];
                                const int prediction = window0->applies._[idxapply].logit >= th;
                                const int infected = window0->windowing->source->wclass.mc > 0;
                                const int is_true = prediction == infected;
                                detections[idxapply]._[idxth].th = th;
                                detections[idxapply]._[idxth].windows[source->wclass.mc][is_true]++;
                                detections[idxapply]._[idxth].sources[source->wclass.mc][source->index.multi][is_true]++;
                            }
                        }
                    } // calculating detections

                    FORBY((*by), apply) { APPLY_SKIP;
                        FORBY((*by), thchooser) { THCHOOSER_SKIP;
                            for (size_t idxth = 0; idxth < ths._[idxapply].number; idxth++) {
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
                    }// calculating performances for each detection and setting the best one

                    for (size_t w = 0; w < split.test->windows.all.number; w++) {
                        RWindow0 window0 = split.test->windows.all._[w];
                        RSource source = window0->windowing->source;

                        FORBY((*by), apply) { APPLY_SKIP;
                            FORBY((*by), thchooser) { THCHOOSER_SKIP;
                                TrainerBy_splits* result = &GETBY5((*by), wsize, apply, fold, try, thchooser).splits._[k];
                                Detection* detection = &result->best_test;
                                double th = best_detections[idxapply]._[idxthchooser].th;

                                const int prediction = window0->applies._[idxapply].logit >= th;
                                const int infected = window0->windowing->source->wclass.mc > 0;
                                const int is_true = prediction == infected;

                                detection->th = th;
                                detection->windows[source->wclass.mc][is_true]++;
                                detection->sources[source->wclass.mc][source->index.multi][is_true]++;
                            }
                        }
                        
                        FORBY((*by), apply) { APPLY_SKIP;
                            FORBY((*by), thchooser) { THCHOOSER_SKIP;
                                TrainerBy_splits* result = &GETBY5((*by), wsize, apply, fold, try, thchooser).splits._[k];

                                result->threshold_chooser = &thchoosers._[idxthchooser];

                                memcpy(&result->best_train, &best_detections[idxapply]._[idxthchooser], sizeof(Detection));
                            }
                        }
                    } // filling Result

                    FORBY((*by), apply) { APPLY_SKIP;
                        FORBY((*by), thchooser) { THCHOOSER_SKIP;
                            TrainerBy_splits* result = &GETBY5((*by), wsize, apply, fold, try, thchooser).splits._[k];
                            {
                                char fpath[PATH_MAX];
                                sprintf(fpath, "results_%ld_%ld_%ld_%ld_%s__%ld.bin", idxfold, idxtry, idxwsize, idxapply, thchoosers._[idxthchooser].name, k);
                                io_path_concat(rootdir, fpath, fpath);
                                trainer_io_results_file(IO_WRITE, fpath, result);
                            }
                        }
                        FREEMANY(best_detections[idxapply]);
                        FREEMANY(detections[idxapply]);
                        FREEMANY(results_todo._[idxapply].thchoosers);
                    }

                    _trainer_ths_free(ths);
                    FREEMANY(results_todo);
                }
            }
        }
    }

    return trainer;
}
void trainer_free(RTrainer trainer) {
    FORBY(trainer->by, wsize) {
        FORBY(trainer->by, apply) {
            FORBY(trainer->by, fold) {
                FORBY(trainer->by, try) {
                    FORBY(trainer->by, thchooser) {
                        FREEMANY(GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits);
                    }
                    printf("\n");
                    FREEMANY(GETBY4(trainer->by, wsize, apply, fold, try).bythchooser);
                }
                FREEMANY(GETBY3(trainer->by, wsize, apply, fold).bytry);
            }
            FREEMANY(GETBY2(trainer->by, wsize, apply).byfold);
        }
        FREEMANY(GETBY(trainer->by, wsize).byapply);
    }
    FREEMANY(trainer->by.bywsize);
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

    FRW(trainer->by.n.wsize);
    FRW(trainer->by.n.apply);
    FRW(trainer->by.n.fold);
    FRW(trainer->by.n.thchooser);

    FORBY(trainer->by, wsize) {
        FORBY(trainer->by, apply) {
            FORBY(trainer->by, fold) {
                FORBY(trainer->by, try) {
                    FORBY(trainer->by, thchooser) {
                        for (size_t k = 0; k < GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits.number; k++) {
                            TrainerBy_splits* result = &GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits._[k];

                            trainer_io_detection(rw, file, &result->best_train);
                            trainer_io_detection(rw, file, &result->best_test);

                            if (IO_READ) {
                                result->threshold_chooser = &trainer->thchoosers._[idxthchooser];
                            }
                        }
                    }
                }
            }
        }
    }
}

int trainer_io_results_file(IOReadWrite rw, char fpath[PATH_MAX], TrainerBy_splits* result) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FILE* file;

    file = io_openfile(rw, fpath);

    if (!file) {
        LOG_DEBUG("%s: File not found: %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath);
        return -1;
    }

    LOG_TRACE("%s: file %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath);

    trainer_io_detection(rw, file, &result->best_train);
    trainer_io_detection(rw, file, &result->best_test);

    return fclose(file);
}

void trainer_io(IOReadWrite rw, char dirname[200], RTestBed2 tb2, RTrainer* trainer) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    char fpath[210];
    FILE* file;
    MANY(Performance) thchoosers;

    sprintf(fpath, "%s/trainer.bin", dirname);

    file = io_openfile(rw, fpath);

    if (rw == IO_WRITE) {
        CLONEMANY(thchoosers, (*trainer)->thchoosers);
    }

    trainer_io_thchoosers(rw, file, &thchoosers);

    if (rw == IO_READ) {
        *trainer = trainer_create(tb2, thchoosers);
    }

    FREEMANY(thchoosers);

    trainer_io_results(rw, file, *trainer);

    fclose(file);

    if (rw == IO_WRITE) {
        LOG_DEBUG("Results saved in: <%s>.", fpath);
        trainer_md(dirname, *trainer);
    }
}

void trainer_md(char dirname[200], RTrainer trainer) {
    char fpath[210];
    TrainerBy* by;
    RTestBed2 tb2;
    FILE* file;

    sprintf(fpath, "%s/trainer.md", dirname);
    file = fopen(fpath, "w");

    by = &trainer->by;
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

        FORBY(trainer->by, wsize) {
            FORBY(trainer->by, apply) {
                FORBY(trainer->by, fold) {
                    FORBY(trainer->by, try) {
                        FORBY(trainer->by, thchooser) {
                            for (size_t k = 0; k < GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits.number; k++) {
                                TrainerBy_splits* result = &GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits._[k];

                                FP("|%*ld",  len_header, trainer->tb2->wsizes._[idxwsize].value);
                                FP("|%*ld",  len_header, idxapply);
                                FP("|%*ld",  len_header, k);
                                FP("|%*s",  len_header, trainer->thchoosers._[idxthchooser].name);
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