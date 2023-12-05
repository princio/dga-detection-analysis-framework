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
#include "cache.h"
#include "common.h"
#include "performance_defaults.h"
#include "kfold0.h"
#include "io.h"
#include "parameters.h"
#include "stratosphere.h"
#include "testbed2.h"
#include "trainer.h"
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

//         for (size_t k = 0; k < tts.number; k++) {
//             for (size_t ev = 0; ev < performances.number; ev++) {
//                 FRW(score[k][0][ev]);
//                 FRW(score[k][1][ev]);
//             }
//         }
//     }
// }

MANY(PSet)* make_parameters() {
    PSetGenerator* psetgenerator = calloc(1, sizeof(PSetGenerator));

    {
        NN nn[] = { NN_NONE };//, NN_TLD, NN_ICANN, NN_PRIVATE };

        Whitelisting whitelisting[] = {
            { .rank = 100, .value = 10 }
            // { .rank = 1000, .value = -50 },
            // { .rank = 100000, .value = -50 },
            // { .rank = 1000000, .value = -10 },  
            // { .rank = 1000000, .value = -50 }
        };

        WindowingType windowing[] = {
            WINDOWING_Q
            // WINDOWING_R,
            // WINDOWING_QR
        };

        InfiniteValues infinitevalues[] = {
            { .ninf = -20, .pinf = 20 }
        };

        double nx_epsilon_increment[] = { 0.1 };

        #define _COPY(A) psetgenerator->A = calloc(1, sizeof(A)); \
        memcpy(psetgenerator->A, A, sizeof(A));
    
        psetgenerator->n_nn = sizeof(nn) / sizeof(NN);
        _COPY(nn);

        psetgenerator->n_whitelisting = sizeof(whitelisting) / sizeof(Whitelisting);
        _COPY(whitelisting);

        psetgenerator->n_windowing = sizeof(windowing) / sizeof(WindowingType);
        _COPY(windowing);

        psetgenerator->n_infinitevalues = sizeof(infinitevalues) / sizeof(InfiniteValues);
        _COPY(infinitevalues);

        psetgenerator->n_nx = sizeof(nx_epsilon_increment) / sizeof(double);
        _COPY(nx_epsilon_increment);

        #undef _COPY
    }

    MANY(PSet)* psets = parameters_generate(psetgenerator);

    free(psetgenerator->nn);
    free(psetgenerator->whitelisting);
    free(psetgenerator->windowing);
    free(psetgenerator->infinitevalues);
    free(psetgenerator->nx_epsilon_increment);
    free(psetgenerator);

    return psets;
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

// void print_score(Score* score) {
//     printf("%10s\t%8.2f\t%5.4f\t", score->performance.name, score->th, score->score);
//     DGAFOR(cl) {
//         Detection d = score->fulldetection[cl];
//         printf("%6d/%-6d\t", d.windows.trues, d.windows.trues + d.windows.falses);
//     }
// }

// void print_detection(Detection* d) {
//     const int32_t tot_test = d->windows.trues + d->windows.falses;
//     printf("(%8d/%-8d)", d->windows.trues, tot_test);
// }

// void print_score_tt(Score* score_train, Score* score_test) {
//     printf("%-15s\t", score_train->performance.name);
//     printf("%8.2f\n", score_train->th);

//     printf("%5.4f", score_train->score);
//     DGAFOR(cl) {
//         printf("\t");
//         print_detection(&score_train->fulldetection[cl]);
//     }

//     printf("\n%5.4f", score_test->score);
//     DGAFOR(cl) {
//         printf("\t");
//         print_detection(&score_test->fulldetection[cl]);
//     }

//     printf("\n");
// }

// void print_score_tt_falses(Score* score_train, Score* score_test) {
//     printf("%-15s\t", score_train->performance.name);
//     printf("%8.2f\t", score_train->th);

//     printf("(%8.4g/%-8.4g)", score_train->score, score_test->score);
//     DGAFOR(cl) {
//         printf("\t(%6d/%-6d)", score_train->fulldetection[cl].windows.falses, score_test->fulldetection[cl].windows.falses);
//     }

//     printf("\n");
// }

// void print_score_tt_relative(Score* score_train, Score* score_test) {
//     printf("%-15s\t", score_train->performance.name);
//     printf("%8.2f\t", score_train->th);

//     #define ROUND4(v) round(v * 10000) / 100

//     printf("(%8.4g/%-8.4g)", ROUND4(score_train->score), ROUND4(score_test->score));
//     DGAFOR(cl) {
//         const double train_pr = ((double) score_train->fulldetection[cl].windows.trues) / (score_train->fulldetection[cl].windows.trues + score_train->fulldetection[cl].windows.falses);
//         const double test_pr = ((double) score_test->fulldetection[cl].windows.trues) / (score_test->fulldetection[cl].windows.trues + score_test->fulldetection[cl].windows.falses);
//         printf("\t(%8.3g/%-8.3g)", ROUND4(train_pr), ROUND4(test_pr));
//     }
//     #undef ROUND4

//     printf("\n");
// }

// void print_tt(TT tt) {
//     printf("TT: ");
//     DGAFOR(cl) {
//         printf("(%d\t%6ld|%-6ld)\t", cl, tt[cl].train.number, tt[cl].test.number);
//     }
//     printf("\n");
// }

// void print_cm(Detection* d) {
//     DGAFOR(cl) {
//         printf("%6d/%-6d\t", d->windows[cl][1], d->windows[cl][1] + d->windows[cl][0]);
//     }
// }

// void print_fulldetection(Detection* d[N_DGACLASSES]) {
//     DGAFOR(cl) {
//         print_cm(d[cl]);
//     }
// }

// void train(TT tt, MANY(Performance) performances, Test tests[performances.number]) {
//     MANY(double) ths;
//     int8_t outputs_init[tt[DGACLASS_0].train.number];
//     double best_scores[performances.number];

//     ths = _rwindows_ths(&tt[DGACLASS_0].train);

//     memset(tests, 0, performances.number * sizeof(Test));
//     memset(outputs_init, 0, tt[DGACLASS_0].train.number * sizeof(int8_t));
//     memset(best_scores, 0, performances.number * sizeof(double));

//     for (int t = 0; t < ths.number; t++) {
//         Detection fulldetection[N_DGACLASSES];

//         DGAFOR(cl) {
//             detect_run(tt[cl].train, ths._[t], &fulldetection[cl]);
//         }

//         for (size_t ev = 0; ev < performances.number; ev++) {
//             double current_score = detect_performance(fulldetection, &performances._[ev]);

//             int is_better = 0;
//             if (outputs_init[ev] == 1) {
//                 is_better = detect_performance_compare(&performances._[ev], current_score, best_scores[ev]);
//             } else {
//                 is_better = 1;
//             }

//             if (is_better) {
//                 outputs_init[ev] = 1;
//                 tests[ev].th = ths._[t];
//                 tests[ev].thchooser = performances._[ev];
//                 memcpy(tests[ev].fulldetections.train, fulldetection, sizeof(fulldetection));
//             }
//         }
//     }

//     free(ths._);
// }

void make_testbed(TestBed2** tb2) {
    size_t index = 0;

    WSize wsizes_arr[] = {
        {.index = index++, .value = 100},
        {.index = index++, .value = 500},
    };

    const size_t n = sizeof(wsizes_arr)/sizeof(WSize);

    MANY(WSize) wsizes;
    INITMANY(wsizes, n, WSize);

    memcpy(wsizes._, wsizes_arr, n * sizeof(WSize));

    *tb2 = testbed2_create(wsizes);

    stratosphere_add(*tb2);

    testbed2_windowing(*tb2);

    free(wsizes._);
}

// void runtraintest(KFold kfold, MANY(Performance) performances) {
//     MANY(TT) tts;

//     Test tests[kfold.config.kfolds][performances.number]; // = calloc(tb->psets.number * performances.number * kconfig.kfolds, sizeof(Test));
//     memset(tests, 0, performances.number * kfold.config.kfolds * sizeof(Test));

//     for (int32_t k = 0; k < kfold.config.kfolds; k++) {
//         for (size_t ev = 0; ev < performances.number; ev++) {
//             tests[k][ev].thchooser = performances._[ev];
//         }
//     }

//     tts = kfold.ks;

//     for (int32_t k = 0; k < kfold.config.kfolds; k++) {
//         Test* tests_k = tests[k];

//         train(tts._[k], performances, tests_k);

//         for (size_t ev = 0; ev < performances.number; ev++) {
//             DGAFOR(cl) {
//                 detect_run(tts._[k][cl].test, tests_k[ev].th, &tests_k[ev].fulldetections.test[cl]);
//             }
//         }
//     }

//     free(performances._);
// }

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

    cache_setroot("/home/princio/Desktop/exps/");

    TestBed2* tb2;

    make_testbed(&tb2);


    // MANY(RWindow0) window0s = tb2->datasets.wsize._[0]->windows.all;
    // for (size_t w = 0; w < window0s.number; w++) {
    //     printf("%d\t%ld\t%ld\n", window0s._[w]->windowing->source->index, window0s._[w]->fn_req_min, window0s._[w]->fn_req_max);
    // }

    MANY(PSet)* psets = make_parameters();

    testbed2_apply(tb2, psets);

    MANY(Performance) performances = gen_performance();

    KFoldConfig0  kconfig0;
    kconfig0.balance_method = KFOLD_BM_NOT_INFECTED;
    kconfig0.kfolds = 5;
    kconfig0.test_folds = 1;
    kconfig0.split_method = KFOLD_SM_MERGE_12;
    kconfig0.shuffle = 1;

    #define TR(CM, CL) ((double) (CM)[CL][1]) / ((CM)[CL][0] + (CM)[CL][1])

    Results* results = trainer_run(tb2, performances, kconfig0);
    for (size_t w = 0; w < tb2->wsizes.number; w++) {
        for (size_t a = 0; a < tb2->psets.number; a++) {
            for (size_t k = 0; k < results->kfolds.wsize.number; k++) {
                for (size_t t = 0; t < results->thchoosers.number; t++) {

                    Result* result = &RESULT_IDX((*results), w, a, t, k);

                    {
                        char headers[4][50];
                        char header[210];
                        size_t h_idx = 0;

                        sprintf(headers[h_idx++], "%5ld", tb2->wsizes._[w].value);
                        sprintf(headers[h_idx++], "%3ld", a);
                        sprintf(headers[h_idx++], "%3ld", k);
                        sprintf(headers[h_idx++], "%12s", results->thchoosers._[t].name);
                        sprintf(header, "%s,%s,%s,%s,", headers[0], headers[1], headers[2], headers[3]);
                        printf("%-30s ", header);
                    }

                    DGAFOR(cl) {
                        if (cl == 1) continue;
                        printf("%1.4f\t", TR(result->best_train.windows, cl));
                    }
                    printf("|\t");
                    DGAFOR(cl) {
                        if (cl == 1) continue;
                        printf("%1.4f\t", TR(result->best_test.windows, cl));
                    }
                    printf("\n");
                }
            }
        }
    }


    char dirname[200] = "/home/princio/Desktop/results/test";
    io_makedir(dirname, 200, 1);
    results_io(IO_WRITE, dirname, results);

    FREEMANY(performances);
    FREEMANYREF(psets);
    free(psets);
    windows_free();
    window0s_free();
    windowings_free();
    sources_free();
    datasets0_free();
    testbed2_free(tb2);
    trainer_free(results);

    return 0;
}