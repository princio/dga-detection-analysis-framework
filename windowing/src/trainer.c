#include "trainer.h"

#include "dataset.h"
#include "io.h"
// #include "logger.h"
#include "configsuite.h"
#include "detect.h"
#include "sources.h"
#include "trainer_detections.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define APPLY_SKIP if (results_todo._[idxconfig].todo == 0) { continue; }
#define THCHOOSER_SKIP if (results_todo._[idxconfig].thchoosers._[idxthchooser] == 0) { continue; }

typedef int TODO;
MAKEMANY(TODO);

typedef struct ResultsTODO {
    int todo;
    MANY(TODO) thchoosers;
} ResultsTODO;

MAKEMANY(ResultsTODO);

void trainer_md(char dirname[200], RTrainer trainer);
int trainer_io_results_file(IOReadWrite rw, char fpath[PATH_MAX], TrainerBy_splits* result);

MANY(ThsDataset) _trainer_ths(RDataset dataset, MANY(ResultsTODO) todo) {
    MANY(ThsDataset) ths;

    MANY_INIT(ths, todo.number, MANY(ThsDataset));

    for (size_t a = 0; a < todo.number; a++) {
        if (todo._[a].todo) {
            MANY_INIT(ths._[a], dataset->windows.all.number, ThsDataset);
        }
    }

    size_t min = SIZE_MAX;
    size_t max = - SIZE_MAX;
    size_t avg = 0;
    int th_reducer;

    th_reducer = 100;
    // if (dataset->windows.all.number < 10000) th_reducer = 25;
    // else
    // if (dataset->windows.all.number < 1000) th_reducer = 10;
    // else
    // if (dataset->windows.all.number < 100) th_reducer = 5;
    
    for (size_t a = 0; a < todo.number; a++) {
        if (todo._[a].todo == 0) {
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

MANY(ThsDataset) _trainer_ths2(RDataset dataset, const size_t n_configs) {
    MANY(ThsDataset) ths;

    MANY_INIT(ths, n_configs, ThsDataset);

    size_t min = SIZE_MAX;
    size_t max = - SIZE_MAX;
    size_t avg = 0;
    int th_reducer;

    th_reducer = 100;
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
    
    printf("Ths number: reducer=%d\twn=%-10ld\tmin=%-10ld\tmax=%-10ld\tavg=%-10ld", th_reducer, dataset->windows.all.number, min, max, avg / n_configs);

    return ths;
}

ThsDataset _trainer_ths_2(RDataset dataset, size_t n_ths) {
    ThsDataset ths;

    MANY_INIT(ths, 0, double);

    double ths_min = DBL_MAX;
    double ths_max = - DBL_MAX;

    size_t applies_number = dataset->windows.all._[0]->applies.number;
    
    for (size_t a = 0; a < applies_number; a++) {
        for (size_t w = 0; w < dataset->windows.all.number; w++) {
            const double logit = dataset->windows.all._[w]->applies._[a].logit;
            if (logit > ths_max) ths_max = logit;
            if (logit < ths_min) ths_min = logit;
        }
    }

    size_t ths_freq[n_ths * 50];
    double ths_freq_rel[n_ths * 50];

    const double cell_width = (ths_max - ths_min) / (n_ths * 50);
    
    for (size_t a = 0; a < applies_number; a++) {
        for (size_t w = 0; w < dataset->windows.all.number; w++) {
            const double logit = dataset->windows.all._[w]->applies._[a].logit;
            size_t ths_freq_i = ((logit - ths_min) / cell_width);
            ths_freq[ths_freq_i]++;
        }
    }

    for (size_t i = 0; i < n_ths * 50; i++) {
        ths_freq_rel[i] = ((double) ths_freq[i]) / (n_ths * 50);
        if (ths_freq_rel[i] > 0) printf("%5ld) %4.4f\n", i, 100 * ths_freq_rel[i]);
    }
    printf("\n");
    
    return ths;
}

void _trainer_ths_free(MANY(ThsDataset) ths) {
    for (size_t i = 0; i < ths.number; i++) {
        FREEMANY(ths._[i]);
    }
    FREEMANY(ths);
}

RTrainer trainer_create(RTB2D tb2d, MANY(Performance) thchoosers) {
    RTrainer trainer;

    trainer = calloc(1, sizeof(__Trainer));

    TrainerBy* by = &trainer->by;

    trainer->tb2d = tb2d;
    trainer->thchoosers = thchoosers;

    CLONEMANY(trainer->thchoosers, thchoosers);

    BY_SETN((*by), config, tb2d->tb2w->configsuite.configs.number);
    BY_SETN((*by), fold, tb2d->n.fold);
    BY_SETN((*by), try, tb2d->n.try);
    BY_SETN((*by), thchooser, thchoosers.number);

    BY_INIT1(trainer->by, config, TrainerBy);
    BY_FOR((*by), config) {
        BY_INIT2(trainer->by, config, fold, TrainerBy);
        BY_FOR((*by), fold) {
            BY_INIT3(trainer->by, config, fold, try, TrainerBy);
            BY_FOR((*by), try) {
                BY_INIT4(trainer->by, config, fold, try, thchooser, TrainerBy);
                BY_FOR((*by), thchooser) {
                    MANY_INIT(BY_GET4((*by), config, fold, try, thchooser).splits, tb2d->folds._[idxfold].k, TrainerBy_splits);
                }
            }
        }
    }

    return trainer;
}

RTrainer trainer_run(RTB2D tb2d, MANY(Performance) thchoosers, char rootdir[DIR_MAX]) {
    RTrainer trainer = trainer_create(tb2d, thchoosers);
    TrainerBy* by = &trainer->by;

    printf("%-7s | ", "fold");
    printf("%-7s | ", "try");
    printf("%-7s | ", "wsize");
    printf("%-7s | ", "splits");
    printf("%-7s\n", "skipped");
    int r = 0;
    BY_FOR((*by), fold) {
        BY_FOR((*by), try) {
            TCPC(DatasetSplits) splits = &BY_GET2((*tb2d), try, fold);
            if (!splits->isok) {
                printf("Warning: skipping folding for fold=%ld, try=%ld, wsize=%ld", idxfold, idxtry, tb2d->tb2w->wsize);
                continue;
            }

            for (size_t k = 0; k < splits->splits.number; k++) {
                printf("%3ld/%-3ld | ", 1+idxfold, tb2d->n.fold);
                printf("%3ld/%-3ld | ", 1+idxtry, tb2d->n.try);
                printf("%3ld/%-3ld | ", 1+k, splits->splits.number);

                // _trainer_ths_2(splits->splits._[k].train, 100);

                DatasetSplit split = splits->splits._[k];
                MANY(ResultsTODO) results_todo;
                MANY(ThsDataset) ths;
                MANY(Detection) detections[by->n.config];
                MANY(Detection) best_detections[by->n.config];
                int best_detections_init[by->n.config][thchoosers.number];
                size_t skipped[2];

                {
                    char fpath[PATH_MAX];
                    MANY_INIT(results_todo, by->n.config, ResultsTODO);
                    skipped[0] = 0;
                    skipped[1] = 0;
                    BY_FOR((*by), config) {
                        MANY_INIT(results_todo._[idxconfig].thchoosers, thchoosers.number, TODO);
                        BY_FOR((*by), thchooser) {
                            sprintf(fpath, "results_%ld_%ld_%ld_%s__%ld.bin", idxfold, idxtry, idxconfig, thchoosers._[idxthchooser].name, k);
                            io_path_concat(rootdir, fpath, fpath);
                            if (trainer_io_results_file(IO_READ, fpath, &BY_GET4((*by), config, fold, try, thchooser).splits._[k])) {
                                results_todo._[idxconfig].thchoosers._[idxthchooser] = 1;
                                results_todo._[idxconfig].todo = 1;
                            } else {
                                skipped[0]++;
                            }
                            skipped[1]++;
                        }
                        if (results_todo._[idxconfig].todo == 0) {
                            FREEMANY(results_todo._[idxconfig].thchoosers);
                        }
                    }
                }

                printf("%3ld/%-3ld\n", skipped[0], skipped[1]);

                memset(best_detections_init, 0, sizeof(int) * by->n.config * thchoosers.number);
                ths = _trainer_ths(split.train, results_todo);

                BY_FOR((*by), config) { APPLY_SKIP;
                    MANY_INIT(detections[idxconfig], ths._[idxconfig].number, Detection);
                    MANY_INIT(best_detections[idxconfig], thchoosers.number, Detection);
                }

                CLOCK_START(calculating_detections);
                for (size_t idxwindow = 0; idxwindow < split.train->windows.all.number; idxwindow++) {
                    RWindow window0 = split.train->windows.all._[idxwindow];
                    RSource source = window0->windowing->source;


                    BY_FOR((*by), config) {
                        APPLY_SKIP;
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
                CLOCK_END(calculating_detections);


                CLOCK_START(calculating_performance_train);
                BY_FOR((*by), config) { APPLY_SKIP;
                    BY_FOR((*by), thchooser) { THCHOOSER_SKIP;
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

                    BY_FOR((*by), config) { APPLY_SKIP;
                        // printf("%ld\t%f\n", );

                        BY_FOR((*by), thchooser) { THCHOOSER_SKIP;
                            TrainerBy_splits* result = &BY_GET4((*by), config, fold, try, thchooser).splits._[k];
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
                    
                    BY_FOR((*by), config) { APPLY_SKIP;
                        BY_FOR((*by), thchooser) { THCHOOSER_SKIP;
                            TrainerBy_splits* result = &BY_GET4((*by), config, fold, try, thchooser).splits._[k];

                            result->threshold_chooser = &thchoosers._[idxthchooser];

                            memcpy(&result->best_train, &best_detections[idxconfig]._[idxthchooser], sizeof(Detection));
                        }
                    }
                } // filling Result
                CLOCK_END(calculating_performance_from_train_th_to_test);

                BY_FOR((*by), config) { APPLY_SKIP;
                    BY_FOR((*by), thchooser) { THCHOOSER_SKIP;
                        TrainerBy_splits* result = &BY_GET4((*by), config, fold, try, thchooser).splits._[k];
                        {
                            char fpath[PATH_MAX];
                            sprintf(fpath, "results_%ld_%ld_%ld_%s__%ld.bin", idxfold, idxtry, idxconfig, thchoosers._[idxthchooser].name, k);
                            io_path_concat(rootdir, fpath, fpath);
                            trainer_io_results_file(IO_WRITE, fpath, result);
                        }
                    }
                    FREEMANY(best_detections[idxconfig]);
                    FREEMANY(detections[idxconfig]);
                    FREEMANY(results_todo._[idxconfig].thchoosers);
                }

                _trainer_ths_free(ths);
                FREEMANY(results_todo);
            }
        }
    }

    return trainer;
}

RTrainer trainer_run2(RTB2D tb2d, MANY(Performance) thchoosers, char rootdir[DIR_MAX]) {
    RTrainer trainer = trainer_create(tb2d, thchoosers);

    trainer_detections_context* context = trainer_detections_start(trainer);

    if (trainer_detections_wait(context)) {
        return NULL;
    }

    return trainer;
}

void trainer_free(RTrainer trainer) {
    BY_FOR(trainer->by, config) {
        BY_FOR(trainer->by, fold) {
            BY_FOR(trainer->by, try) {
                BY_FOR(trainer->by, thchooser) {
                    FREEMANY(BY_GET4(trainer->by, config, fold, try, thchooser).splits);
                }
                FREEMANY(BY_GET3(trainer->by, config, fold, try).bythchooser);
            }
            FREEMANY(BY_GET2(trainer->by, config, fold).bytry);
        }
        FREEMANY(BY_GET(trainer->by, config).byfold);
    }
    FREEMANY(trainer->by.byconfig);
    FREEMANY(trainer->thchoosers);
    free(trainer);
}

void trainer_io_thchoosers(IOReadWrite rw, FILE* file, MANY(Performance)* thchoosers) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(thchoosers->number);

    if (rw == IO_READ) {
        MANY_INITREF(thchoosers, thchoosers->number, Performance);
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

    FRW(trainer->by.n.config);
    FRW(trainer->by.n.fold);
    FRW(trainer->by.n.thchooser);

    BY_FOR(trainer->by, config) {
        BY_FOR(trainer->by, fold) {
            BY_FOR(trainer->by, try) {
                BY_FOR(trainer->by, thchooser) {
                    for (size_t k = 0; k < BY_GET4(trainer->by, config, fold, try, thchooser).splits.number; k++) {
                        TrainerBy_splits* result = &BY_GET4(trainer->by, config, fold, try, thchooser).splits._[k];

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

void trainer_io(IOReadWrite rw, char dirname[200], RTB2D tb2d, RTrainer* trainer) {
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
        *trainer = trainer_create(tb2d, thchoosers);
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
    RTB2W tb2w;
    FILE* file;

    sprintf(fpath, "%s/trainer.md", dirname);
    file = fopen(fpath, "w");

    by = &trainer->by;
    tb2w = trainer->tb2d->tb2w;

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

        BY_FOR(trainer->by, config) {
            BY_FOR(trainer->by, fold) {
                BY_FOR(trainer->by, try) {
                    BY_FOR(trainer->by, thchooser) {
                        for (size_t k = 0; k < BY_GET4(trainer->by, config, fold, try, thchooser).splits.number; k++) {
                            TrainerBy_splits* result = &BY_GET4(trainer->by, config, fold, try, thchooser).splits._[k];

                            FP("|%*ld",  len_header, idxconfig);
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

    #undef FP
    #undef FPNL
    #undef TP

    fclose(file);
}