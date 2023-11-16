/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main file of the project
 *
 *        Created:  03/24/2016 19:40:56
 *
 *         Author:  Gustavo Pantuza
 *   Organization:  Software Community
 *
 * ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "colors.h"
#include "dataset.h"
#include "cache.h"
#include "common.h"
#include "performance_defaults.h"
#include "kfold.h"
#include "io.h"
#include "parameters.h"
#include "stratosphere.h"
#include "result.h"
#include "tt.h"
#include "testbed.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* for ntohl/htonl */
#include <netinet/in.h>
#include <arpa/inet.h>


#include <endian.h>

char WINDOWING_NAMES[3][10] = {
    "QUERY",
    "RESPONSE",
    "BOTH"
};

char NN_NAMES[11][10] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "ICANN",
    "NONE",
    "PRIVATE",
    "TLD"
};

typedef struct Score {
    double th;
    double score;
    Performance performance;
    Detection fulldetection[N_DGACLASSES];
} Score;

MANY(Performance) performances;

MAKEMANY(double);

// void result_save(PSet* pset, MANY(TT) tts, MANY(Performance) performances, Score score[tts.number][2][performances.number]) {
//     FILE* file;
//     char path[500]; {
//         char subdigest[10];
//         time_t t = time(NULL);
//         struct tm tm = *localtime(&t);

//         strncpy(subdigest, pset->digest, sizeof subdigest);
//         subdigest[9] = '\0';

//         sprintf(path, "%s/result_%s_%d%02d%02d_%02d%02d%02d", CACHE_PATH, subdigest, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
//     }

//     int rw = IO_WRITE;

//     file = io_openfile(rw, file);
//     if (file) {
//         FRWNPtr fn = rw ? io_freadN : io_fwriteN;

//         FRW(tts.number);
//         FRW(performances.number);

//         for (int32_t k = 0; k < tts.number; k++) {
//             for (int32_t ev = 0; ev < performances.number; ev++) {
//                 FRW(score[k][0][ev]);
//                 FRW(score[k][1][ev]);
//             }
//         }
//     }
// }

PSetGenerator* gen_psetgenerator() {
    PSetGenerator* psetgenerator = calloc(1, sizeof(PSetGenerator));

    {
        int32_t wsize[] = { 1, 100 }; //, 5, 100 };//, 5, 100, 2500 };
        //     wsizes._[1] = 50;
        //     wsizes._[2] = 100;
        //     wsizes._[3] = 200;
        //     wsizes._[4] = 2500;

        NN nn[] = { NN_NONE };//, NN_TLD, NN_ICANN, NN_PRIVATE };

        Whitelisting whitelisting[] = {
            { .rank = 0, .value = 0 },
            // { .rank = 1000, .value = -50 },
            // { .rank = 100000, .value = -50 },
            // { .rank = 1000000, .value = -10 },  
            // { .rank = 1000000, .value = -50 }
        };

        WindowingType windowing[] = {
            WINDOWING_Q,
            // WINDOWING_R,
            // WINDOWING_QR
        };

        InfiniteValues infinitevalues[] = {
            { .ninf = -20, .pinf = 20 }
        };

        #define _COPY(A) psetgenerator->A = calloc(1, sizeof(A)); \
        memcpy(psetgenerator->A, A, sizeof(A));
    
        psetgenerator->n_wsize = sizeof(wsize) / sizeof(int32_t);
        _COPY(wsize);

        psetgenerator->n_nn = sizeof(nn) / sizeof(NN);
        _COPY(nn);

        psetgenerator->n_whitelisting = sizeof(whitelisting) / sizeof(Whitelisting);
        _COPY(whitelisting);

        psetgenerator->n_windowing = sizeof(windowing) / sizeof(WindowingType);
        _COPY(windowing);

        psetgenerator->n_infinitevalues = sizeof(infinitevalues) / sizeof(InfiniteValues);
        _COPY(infinitevalues);

        #undef _COPY
    }

    return psetgenerator;
}

void free_psetgenerator(PSetGenerator* psetgenerator) {
    free(psetgenerator->wsize);
    free(psetgenerator->nn);
    free(psetgenerator->whitelisting);
    free(psetgenerator->windowing);
    free(psetgenerator->infinitevalues);
    free(psetgenerator);
}

MANY(double) _rwindows_ths(const DGAMANY(RWindow) dsrwindows) {
    MANY(double) ths;
    int max;
    double* ths_tmp;

    max = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        max += dsrwindows[cl].number;
    }

    ths_tmp = calloc(max, sizeof(double));

    int n = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        for (int32_t w = 0; w < dsrwindows[cl].number; w++) {
            int logit;
            int exists;

            logit = floor(dsrwindows[cl]._[w]->logit);
            exists = 0;
            for (int32_t i = 0; i < n; i++) {
                if (ths_tmp[i] == logit) {
                    exists = 1;
                    break;
                }
            }

            if(!exists) {
                ths_tmp[n++] = logit;
            }
        }
    }

    ths.number = n;
    ths._ = calloc(n, sizeof(double));
    memcpy(ths._, ths_tmp, sizeof(double) * n);

    free(ths_tmp);

    return ths;
}

MANY(Performance) gen_performance() {
    MANY(Performance) performances;

    INITMANY(performances, 3, Performance);

    int i = 0;
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_1];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_05];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_01];
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_TPR];
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_FPR];

    return performances;
}

void print_score(Score* score) {
    printf("%10s\t%8.2f\t%5.4f\t", score->performance.name, score->th, score->score);
    DGAFOR(cl) {
        Detection d = score->fulldetection[cl];
        printf("%6d/%-6d\t", d.windows.trues, d.windows.trues + d.windows.falses);
    }
}

void print_detection(Detection* d) {
    const int32_t tot_test = d->windows.trues + d->windows.falses;
    printf("(%8d/%-8d)", d->windows.trues, tot_test);
}

void print_score_tt(Score* score_train, Score* score_test) {
    printf("%-15s\t", score_train->performance.name);
    printf("%8.2f\n", score_train->th);

    printf("%5.4f", score_train->score);
    DGAFOR(cl) {
        printf("\t");
        print_detection(&score_train->fulldetection[cl]);
    }

    printf("\n%5.4f", score_test->score);
    DGAFOR(cl) {
        printf("\t");
        print_detection(&score_test->fulldetection[cl]);
    }

    printf("\n");
}

void print_score_tt_falses(Score* score_train, Score* score_test) {
    printf("%-15s\t", score_train->performance.name);
    printf("%8.2f\t", score_train->th);

    printf("(%8.4g/%-8.4g)", score_train->score, score_test->score);
    DGAFOR(cl) {
        printf("\t(%6d/%-6d)", score_train->fulldetection[cl].windows.falses, score_test->fulldetection[cl].windows.falses);
    }

    printf("\n");
}

void print_score_tt_relative(Score* score_train, Score* score_test) {
    printf("%-15s\t", score_train->performance.name);
    printf("%8.2f\t", score_train->th);

    #define ROUND4(v) round(v * 10000) / 100

    printf("(%8.4g/%-8.4g)", ROUND4(score_train->score), ROUND4(score_test->score));
    DGAFOR(cl) {
        const double train_pr = ((double) score_train->fulldetection[cl].windows.trues) / (score_train->fulldetection[cl].windows.trues + score_train->fulldetection[cl].windows.falses);
        const double test_pr = ((double) score_test->fulldetection[cl].windows.trues) / (score_test->fulldetection[cl].windows.trues + score_test->fulldetection[cl].windows.falses);
        printf("\t(%8.3g/%-8.3g)", ROUND4(train_pr), ROUND4(test_pr));
    }
    #undef ROUND4

    printf("\n");
}

void print_tt(TT tt) {
    printf("TT: ");
    DGAFOR(cl) {
        printf("(%d\t%6d|%-6d)\t", cl, tt[cl].train.number, tt[cl].test.number);
    }
    printf("\n");
}

void print_cm(Detection* d) {
    printf("%6d/%-6d\t", d->windows.trues, d->windows.trues + d->windows.falses);
}

void print_fulldetection(Detection* d[N_DGACLASSES]) {
    DGAFOR(cl) {
        print_cm(d[cl]);
    }
}

void train(TT tt, MANY(Performance) performances, Test tests[performances.number]) {
    MANY(double) ths;
    int8_t outputs_init[tt[DGACLASS_0].train.number];
    double best_scores[performances.number];

    ths = _rwindows_ths(&tt[DGACLASS_0].train);

    memset(tests, 0, performances.number * sizeof(Test));
    memset(outputs_init, 0, tt[DGACLASS_0].train.number * sizeof(int8_t));
    memset(best_scores, 0, performances.number * sizeof(double));

    for (int t = 0; t < ths.number; t++) {
        Detection fulldetection[N_DGACLASSES];

        DGAFOR(cl) {
            detect_run(tt[cl].train, ths._[t], &fulldetection[cl]);
        }

        for (int32_t ev = 0; ev < performances.number; ev++) {
            double current_score = detect_performance(fulldetection, &performances._[ev]);

            int is_better = 0;
            if (outputs_init[ev] == 1) {
                is_better = detect_performance_compare(&performances._[ev], current_score, best_scores[ev]);
            } else {
                is_better = 1;
            }

            if (is_better) {
                outputs_init[ev] = 1;
                tests[ev].th = ths._[t];
                tests[ev].thchooser = performances._[ev];
                memcpy(tests[ev].fulldetections.train, fulldetection, sizeof(fulldetection));
            }
        }
    }

    free(ths._);
}

void exps_1() {
    PSetGenerator* psetgenerator;
    TestBed* tb;
    KFoldConfig kconfig;
    MANY(Performance) performances;
    MANY(TT) tts;
    
    performances = gen_performance();
    psetgenerator = gen_psetgenerator();

    cache_setpath("/home/princio/Desktop/exps/refactor/exp_6");

    testbed_init(psetgenerator);

    free_psetgenerator(psetgenerator);

    stratosphere_run();

    tb = testbed_load("/tmp/testbed/");
    if(tb == NULL) {
        tb = testbed_run();
        testbed_save(tb, "/tmp/testbed/");
    }

    kconfig.balance_method = FOLDING_DSBM_NOT_INFECTED;
    kconfig.kfolds = 10;
    kconfig.shuffle = 0;
    kconfig.split_method = FOLDING_DSM_IGNORE_1;
    kconfig.test_folds = 5;

    // Score worst[performances.number];
    // Score better[performances.number];
    // Score avg[performances.number];
    
    // for (int32_t ev = 0; ev < performances.number; ev++) {
    //     worst[ev].performance = performances._[ev];
    //     better[ev].performance = performances._[ev];
    //     avg[ev].performance = performances._[ev];

    //     worst[ev].score = performances._[ev].greater_is_better ? 0 : 1;
    //     better[ev].score = performances._[ev].greater_is_better ? 1 : 0;
    //     avg[ev].score = 0;
    // }

    Test tests[tb->psets.number][kconfig.kfolds][performances.number]; // = calloc(tb->psets.number * performances.number * kconfig.kfolds, sizeof(Test));
    memset(tests, 0, tb->psets.number * performances.number * kconfig.kfolds * sizeof(Test));

    for (int32_t p = 0; p < tb->psets.number; p++) {
        for (int32_t k = 0; k < kconfig.kfolds; k++) {
            for (int32_t ev = 0; ev < performances.number; ev++) {
                tests[p][k][ev].thchooser = performances._[ev];
            }
        }
    }
    for (int32_t p = 0; p < tb->psets.number; p++) {
        tts = kfold_run(tb->applies[p].windows.multi, kconfig);

        for (int32_t k = 0; k < kconfig.kfolds; k++) {
            Test* tests_pk = tests[p][k];

            train(tts._[k], performances, tests_pk);

            for (int32_t ev = 0; ev < performances.number; ev++) {
                DGAFOR(cl) {
                    detect_run(tts._[k][cl].test, tests_pk[ev].th, &tests_pk[ev].fulldetections.test[cl]);
                }
            }
        }

        kfold_free(tts);
    }

    for (int32_t p = 0; p < tb->psets.number; p++) {
        for (int32_t k = 0; k < kconfig.kfolds; k++) {
            for (int32_t ev = 0; ev < performances.number; ev++) {
                // tests[p][k][ev].fulldetections.test
            }
        }
    }

    // for (int32_t ev = 0; ev < performances.number; ev++) {
    //     printf("%-30s\t%8.4f\t%8.f\n", performances._[ev].name, worst[ev].score, better[ev].score);
    // }

    testbed_free(tb);
    free(performances._);
}

int main (int argc, char* argv[]) {
    setbuf(stdout, NULL);

    /* Read command line options */
    options_t options;
    options_parser(argc, argv, &options);

#ifdef DEBUG
    fprintf(stdout, BLUE "Command line options:\n" NO_COLOR);
    fprintf(stdout, BROWN "help: %d\n" NO_COLOR, options.help);
    fprintf(stdout, BROWN "version: %d\n" NO_COLOR, options.version);
    fprintf(stdout, BROWN "use colors: %d\n" NO_COLOR, options.use_colors);
    fprintf(stdout, BROWN "filename: %s\n" NO_COLOR, options.file_name);
#endif

    /* Do your magic here :) */

    exps_1();

    return 0;
}


/*
    Datasets datasets;
    datasets.number = psets.number;
    datasets._ = calloc(datasets.number, sizeof(Dataset));

    int32_t d = 0;
    for (int32_t p = 0; p < psets.number; p++) {
        datasets._[d].pset = &psets._[p];
        ++d;
    }

    experiment_set_name("exp_new_4");
    experiment_set_path("/home/princio/Desktop/exps");
    experiment_set_psets(psets);

    stratosphere_add_to_galaxyes();

    stratosphere_run(&exp, datasets, &stratosphere_sources);

    experiment_sources_fill(&exp);
    
    for (int32_t i = 0; i < datasets.number; i++) {
        for (int32_t w = 0; w < datasets._[i].windows[1].number; w++) {
            Window window = datasets._[i].windows[1]._[w];
            printf("(%d, %d, %d, %d, %d, %g)\n", i, 1, w, window.source_id, window.window_id, window.logit);
        }
    }

    DATASETs ARE READY
    TEST FOR EACH DATASET, WHICH MEAN:
      - split train test
      - choose with some method (f1score, fpr) the threshold within the train subset
      - test with the choosen threshold the train subset
      - analyze the confusion matricies obtained within the test subset

    printf("Sources loaded\n\n");

    MANY(Performance) perfomances = gen_performance();

    for (int32_t d = 0; d < 0; d++) {
        {
            printf("Processing dataset %d:\t", d);
            PSet* pset = datasets._[d].pset;
            printf("%8d, ", pset->wsize);
            printf("(%5g, %5g), ", pset->infinite_values.ninf, pset->infinite_values.pinf);
            printf("%6s, ", NN_NAMES[pset->nn]);
            printf("(%6d, %6g)\n", pset->whitelisting.rank, pset->whitelisting.value);
        }

        const KFoldConfig kfoldconfig = {
            .kfolds = 2,
            .balance_method = FOLDING_DSBM_NOT_INFECTED,
            .split_method = FOLDING_DSM_IGNORE_1,
            .test_folds = 1,
            .shuffle = 0
        };

        KFold kfold = kfold_run(&datasets._[d], kfoldconfig, perfomances);

        kfold_free(kfold);
    }

    free(psets._);

    sources_lists_free(exp.sources.lists);
    sources_arrays_free(exp.sources.arrays);
    free(stratosphere_sources._);

    for (int32_t i = 0; i < datasets.number; i++) {
        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
            free(datasets._[i].windows[cl]._);
        }
    }

    free(datasets._);

    free(perfomances._);
}
*/
    // Sources sources;

    // Sources sources_by_type[1];

    // Sources sources_by_type[0] = stratosphere_get_sources();

    // int32_t n_sources = 0;
    // for (size_t i = 0; i < 1; i++) {
    //     for (size_t s = 0; s < sources_by_type->number; s++) {
    //         n_sources += sources_by_type[i].number;
    //     }
    // }

    // sources.number = n_sources;
    // sources._ = calloc(n_sources, sizeof(Source));

    // n_sources = 0;
    // for (size_t i = 0; i < 1; i++) {
    //     memcpy(&sources._[n_sources], sources_by_type[i]._, sources_by_type[i].number * sizeof(Source));
    //     n_sources += sources_by_type[i].number;
    // }
    

    // ExperimentSet es;
    // Experiment exp;

    // strcpy(exp.name, "exp_5");
    // strcpy(exp.rootpath, "/home/princio/Desktop/exps");

    // exp.psets = parameters_generate(&psetgenerator);
    // exp.KFOLDs = 10;
    // exp.wsizes = wsizes;

    // for (int32_t s = 0; s < sources.number; s++) {
    //     for (int32_t i = 0; i < wsizes.number; i++) {
    //         sources._[s].fetch(sources._[s], wsizes._[i]);
    //     }
    // }
    

    // es.KFOLDs = 5;
    // es.N_SPLITRATIOs = 2;
    // {
    //     int i = 0;
    //     es.split_percentages[i++] = 0.01;
    //     es.split_percentages[i++] = 0.5;
    //     // es.split_percentages[i++] = 0.1;
    //     // es.split_percentages[i++] = 0.2;
    //     // es.split_percentages[i++] = 0.25;
    //     // es.split_percentages[i++] = 0.3;
    //     // es.split_percentages[i++] = 0.4;
    //     // es.split_percentages[i++] = 0.5;
    //     // es.split_percentages[i++] = 0.6;
    //     // es.split_percentages[i++] = 0.7;
    // }

    // es.evmfs.number = 5;
    // es.evmfs._ = calloc(5, sizeof(EvaluationMetricFunction));
    // int i = 0;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_f1score_beta1;
    // sprintf(es.evmfs._[i].name, "f1score_1");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_f1score_beta05;
    // sprintf(es.evmfs._[i].name, "f1score_05");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_f1score_beta01;
    // sprintf(es.evmfs._[i].name, "f1score_01");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_fpr;
    // sprintf(es.evmfs._[i].name, "fpr");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_tpr;
    // sprintf(es.evmfs._[i].name, "tpr");
    // ++i;

    // experiment_test(&es);

// int ff(const int cursor[4], const int sizes[4], int avg) {
//     const int ncursor = 4;
//     int active[ncursor];
//     int last = 0;

//     for (int i = 0; i < ncursor; i++) active[i] = 0;

//     active[0] = 1;
//     for (int i = 1; i < ncursor; i++) {
//         active[i] = avg & (1 << (i-1)) ? 1 : 0;
//         last = active[i] ? i : last;
//     }

//     int c = 0;
//     for (int l = 0; l < last; l++) {
//         int row = 1;
//         if (cursor[l] == 0) continue;
//         if (active[l] == 0) continue;
//         for (int ll = l + 1; ll <= last; ll++) {
//             if (active[ll]) row *= sizes[ll];
//         }
//         c += row * cursor[l];
//     }
    
//     c += cursor[last];

//     return c;
// }
/*
    int n0 = 3;
    int n1 = 1;
    int n2 = 5;
    int n4 = 237;

    int cm0[n0];
    int cm1[n0][n1];
    int cm2[n0][n2];
    int cm3[n0][n1][n2];
    int cm4[n0][n4];
    int cm5[n0][n1][n4];
    int cm6[n0][n2][n4];
    int cm7[n0][n1][n2][n4];

    for (int i0 = 0; i0 < n0; i0++) {
        for (int i1 = 0; i1 < n1; i1++) {
            for (int i2 = 0; i2 < n2; i2++) {
                for (int i4 = 0; i4 < n4; i4++) {
                    cm0[i0] = rand() % 1000;
                    cm1[i0][i1] = rand() % 1000;
                    cm2[i0][i2] = rand() % 1000;
                    cm3[i0][i1][i2] = rand() % 1000;
                    cm4[i0][i4] = rand() % 1000;
                    cm5[i0][i1][i4] = rand() % 1000;
                    cm6[i0][i2][i4] = rand() % 1000;
                    cm7[i0][i1][i2][i4] = rand() % 1000;
                }
            }
        }
    }
    
    const int sizes[4] = { n0, n1, n2, n4 };

    int cursor[4];

    int wrongs = 0;
    for (int i0 = 0; i0 < n0; i0++) {
        for (int i1 = 1; i1 < n1; i1++) {
            for (int i2 = 0; i2 < n2; i2++) {
                for (int i4 = 1; i4 < n4; i4++) {
                    cursor[0] = i0;
                    cursor[1] = i1;
                    cursor[2] = i2;
                    cursor[3] = i4;
                    int avg = 0;
                    {
                        int a = cm0[i0];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm0)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += !(a == v);
                    }
                    {
                        avg = 4;
                        int a = cm4[i0][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm4)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 2;
                        int a = cm2[i0][i2];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm2)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 6;
                        int a = cm6[i0][i2][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm6)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 1;
                        int a = cm1[i0][i1];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm1)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 5;
                        int a = cm5[i0][i1][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm5)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 3;
                        int a = cm3[i0][i1][i2];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm3)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 7;
                        int a = cm7[i0][i1][i2][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm7)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }
                }
            }
        }
    }


    printf("%d\n", wrongs);


    exit(0);


    // int N_SPLITRATIOs = 10;

    // double split_percentages[10] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7 };
    // int split_nwindows[N_SPLITRATIOs][3];

    // int KFOLDs = 50;

    // memset(split_nwindows, 0, N_SPLITRATIOs * 3 * sizeof(int));

    // Dataset *dt = calloc(windowing->wsizes.number, sizeof(Dataset));
    // dataset_fill(windowing, dt);

    // ConfusionMatrix cm[N_SPLITRATIOs][windowing->wsizes.number][KFOLDs][windowing->psets.number];

    // for (int p = 0; p < N_SPLITRATIOs; ++p) {
    //     double split_percentage = split_percentages[p];
    //     for (int w = 0; w < windowing->wsizes.number; ++w) {
    //         const int32_t wsize = windowing->wsizes._[w];
    //         for (int k = 0; k < KFOLDs; ++k) {
    //             DatasetTrainTest dt_tt;
    //             dataset_traintestsplit(&dt[w], &dt_tt, split_percentage);

    //             double ths[windowing->psets.number];
    //             memset(ths, 0, sizeof(double) * windowing->psets.number);

    //             WindowRefs* ws = &dt_tt.train[CLASS__NOT_INFECTED];
    //             for (int i = 0; i < ws->number; i++)
    //             {
    //                 Window* window = ws->_[i];
    //                 for (int m = 0; m < window->metrics.number; m++) {
    //                     double logit = window->metrics._[m].logit;
    //                     if (ths[m] < logit) ths[m] = logit;
    //                 }
    //             }

    //             dataset_traintestsplit_cm(wsize, &windowing->psets, &dt_tt, windowing->psets.number, ths, &cm[p][w][k][0]); 
    //         }
    //     }
    // }


    // {
    //     const int32_t N_WSIZEs = windowing->wsizes.number;
    //     const int32_t N_METRICs = windowing->psets.number;

    //     #define CMAVG_INIT(N, ID)  \
    //     cmavgs[ID]._ = (CM*) cms_ ## ID;\
    //     cmavgs[ID].separate_id = (ID);\
    //     cmavgs[ID].totals = (N);\
    //     memset(cms_ ## ID, 0, sizeof(CM) * (N));

    //     CMAVG cmavgs[8];

    //     CM cms_0[N_SPLITRATIOs];
    //     CMAVG_INIT(N_SPLITRATIOs, 0);

    //     CM cms_1[N_SPLITRATIOs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_METRICs, 1);

    //     CM cms_2[N_SPLITRATIOs][KFOLDs];
    //     CMAVG_INIT(N_SPLITRATIOs * KFOLDs, 2);

    //     CM cms_3[N_SPLITRATIOs][KFOLDs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * KFOLDs * N_METRICs, 3);

    //     CM cms_4[N_SPLITRATIOs][N_WSIZEs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs, 4);

    //     CM cms_5[N_SPLITRATIOs][N_WSIZEs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * N_METRICs, 5);

    //     CM cms_6[N_SPLITRATIOs][N_WSIZEs][KFOLDs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs, 6);

    //     CM cms_7[N_SPLITRATIOs][N_WSIZEs][KFOLDs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs, 7);


    //     // ( SPLITs=0!, WSIZEs, KFOLDs, METRICs )
    //     // 1 means separating
    //     // 0: ( 1, 0, 0, 0 )   |   SPLIT                        | WSIZE * METRICs * KFOLD
    //     // 1: ( 1, 0, 0, 1 )   |   SPLIT, METRICs               | WSIZE * METRICs * KFOLD
    //     // 2: ( 1, 0, 1, 0 )   |   SPLIT, KFOLD                 |
    //     // 3: ( 1, 0, 1, 1 )   |   SPLIT, KFOLD, METRICs        |
    //     // 4: ( 1, 1, 0, 0 )   |   SPLIT, WSIZE                 |
    //     // 5: ( 1, 1, 0, 1 )   |   SPLIT, WSIZE, METRIC         |
    //     // 6: ( 1, 1, 1, 0 )   |   SPLIT, WSIZE, KFOLD          | 
    //     // 7: ( 1, 1, 1, 1 )   |   SPLIT, WSIZE, KFOLD, METRIC  | 
    //     for (int s = 0; s < N_SPLITRATIOs; ++s) {
    //         for (int w = 0; w < N_WSIZEs; ++w) {
    //             for (int k = 0; k < KFOLDs; ++k) {
    //                 for (int m = 0; m < N_METRICs; ++m) {
    //                     for (int cl = 0; cl < N_DGACLASSES; ++cl) {
    //                         int falses = cm[s][w][k][m].classes[cl][0];
    //                         int trues = cm[s][w][k][m].classes[cl][1];
    //                         { // 
    //                             cms_0[s].classes[cl][0] += falses;
    //                             cms_0[s].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_1[s][m].classes[cl][0] += falses;
    //                             cms_1[s][m].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_2[s][k].classes[cl][0] += falses;
    //                             cms_2[s][k].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_3[s][k][m].classes[cl][0] += falses;
    //                             cms_3[s][k][m].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_4[s][w].classes[cl][0] += falses;
    //                             cms_4[s][w].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_5[s][w][m].classes[cl][0] += falses;
    //                             cms_5[s][w][m].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_6[s][w][k].classes[cl][0] += falses;
    //                             cms_6[s][w][k].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_7[s][w][k][m].classes[cl][0] += falses;
    //                             cms_7[s][w][k][m].classes[cl][1] += trues;
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }

    //     const int32_t MAX = N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs;
    //     for (int i = 0; i < MAX; ++i) {
    //         for (int k = 0; k < 8; ++k) {
    //             for (int cl = 0; cl < N_DGACLASSES; ++cl) {
    //                 if (i < cmavgs[k].totals) {
    //                     cmavgs[k]._[i].classes[cl][0] /= cmavgs[k].totals;
    //                     cmavgs[k]._[i].classes[cl][1] /= cmavgs[k].totals;
    //                 }
    //             }
    //         }
    //     }
    // }

    return EXIT_SUCCESS;
}
*/