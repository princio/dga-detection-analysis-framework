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
#include "common.h"
#include "detect.h"
#include "fold.h"
#include "gatherer.h"
#include "io.h"
#include "logger.h"
#include "parameters.h"
#include "performance_defaults.h"
#include "stratosphere.h"
#include "stat.h"
#include "testbed2.h"
#include "testbed2_io.h"
#include "trainer.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <linux/limits.h>
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

void print_trainer(RTrainer trainer) {

    FILE* file_csv = fopen("results.csv", "w");

    TrainerBy* by = &trainer->by;

    RTestBed2 tb2 = trainer->tb2;


    fprintf(file_csv, ",,,,,,train,train,train,test,test,test");
    fprintf(file_csv, "fold,try,k,wsize,apply,thchooser,0,1,2,0,1,2");


    FORBY(trainer->by, wsize) {
        FORBY(trainer->by, apply) {
            FORBY(trainer->by, fold) {
                FORBY(trainer->by, try) {
                    FORBY(trainer->by, thchooser) {
                        for (size_t k = 0; k < GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits.number; k++) {
                            TrainerBy_splits* result = &GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits._[k];

                            {
                                char headers[4][50];
                                char header[210];
                                size_t h_idx = 0;

                                sprintf(headers[h_idx++], "%3ld", idxfold);
                                sprintf(headers[h_idx++], "%3ld", idxtry);
                                sprintf(headers[h_idx++], "%3ld", k);
                                sprintf(headers[h_idx++], "%5ld", tb2->wsizes._[idxwsize].value);
                                sprintf(headers[h_idx++], "%3ld", idxapply);
                                sprintf(headers[h_idx++], "%12s", trainer->thchoosers._[idxthchooser].name);
                                sprintf(header, "%s,%s,%s,%s,", headers[0], headers[1], headers[2], headers[3]);
                                printf("%-30s ", header);
                            }

                            DGAFOR(cl) {
                                printf("%1.4f\t", DETECT_TRUERATIO(result->best_train, cl));
                            }
                            printf("|\t");
                            DGAFOR(cl) {
                                printf("%1.4f\t", DETECT_TRUERATIO(result->best_test, cl));
                            }
                            printf("\n");
                        }
                    }
                }
            }
        }
    }
}

void csv_trainer(char dirname[PATH_MAX], RTrainer trainer) {
    char fpath[PATH_MAX];
    FILE* file_csv;

    if (io_makedir(dirname, 0)) {
        printf("Error: impossible to create directory <%s>\n", dirname);
        return;
    }

    sprintf(fpath, "%s/results.csv", dirname);
    file_csv = fopen(fpath, "w");

    TrainerBy* by = &trainer->by;

    RTestBed2 tb2 = trainer->tb2;

    fprintf(file_csv, "caos,caos,caos,wsize,pset,pset,pset,pset,pset,pset,pset,pset,train,train,train,test,test,test\n");
    fprintf(file_csv, "fold,try,k,wsize,ninf,pinf,nn,windowing,wl_rank,wl_value,nx_eps_incr,thchooser,0,1,2,0,1,2\n");

    FORBY(trainer->by, wsize) {
        FORBY(trainer->by, apply) {
            FORBY(trainer->by, fold) {
                FORBY(trainer->by, try) {
                    FORBY(trainer->by, thchooser) {
                        for (size_t k = 0; k < GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits.number; k++) {
                            TrainerBy_splits* result = &GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits._[k];

                            fprintf(file_csv, "%ld,", idxfold);
                            fprintf(file_csv, "%ld,", idxtry);
                            fprintf(file_csv, "%ld,", k);
                            fprintf(file_csv, "%ld,", tb2->wsizes._[idxwsize].value);
                            fprintf(file_csv, "%f,", tb2->applies._[idxapply].pset.ninf);
                            fprintf(file_csv, "%f,", tb2->applies._[idxapply].pset.pinf);
                            fprintf(file_csv, "%s,", NN_NAMES[tb2->applies._[idxapply].pset.nn]);
                            fprintf(file_csv, "%s,", WINDOWING_NAMES[tb2->applies._[idxapply].pset.windowing]);
                            fprintf(file_csv, "%ld,", tb2->applies._[idxapply].pset.wl_rank);
                            fprintf(file_csv, "%f,", tb2->applies._[idxapply].pset.wl_value);
                            fprintf(file_csv, "%f,", tb2->applies._[idxapply].pset.nx_epsilon_increment);
                            fprintf(file_csv, "%s", trainer->thchoosers._[idxthchooser].name);

                            DGAFOR(cl) {
                                fprintf(file_csv, ",%1.4f", DETECT_TRUERATIO(result->best_train, cl));
                            }
                            DGAFOR(cl) {
                                fprintf(file_csv, ",%1.4f", DETECT_TRUERATIO(result->best_test, cl));
                            }
                            fprintf(file_csv, "\n");
                        }
                    }
                }
            }
        }
    }
    fclose(file_csv);
}
#undef TR

MANY(WSize) make_wsizes() {
    MANY(WSize) wsizes;
    size_t index;
    size_t n;

    index = 0;

    WSize wsizes_arr[] = {
        // {.index = index++, .value = 1},
        // {.index = index++, .value = 10},
        {.index = index++, .value = 50},
        // {.index = index++, .value = 100},
        // {.index = index++, .value = 500},
        // {.index = index++, .value = 1000},
        // {.index = index++, .value = 1500},
    };

    n = sizeof(wsizes_arr)/sizeof(WSize);
    INITMANY(wsizes, n, WSize);
    memcpy(wsizes._, wsizes_arr, n * sizeof(WSize));

    return wsizes;
}

MANY(PSet) make_parameters(const size_t subset) {
    PSetGenerator* psetgenerator = calloc(1, sizeof(PSetGenerator));

    {
        NN nn[] = {
            NN_NONE,
            // NN_TLD,
            // NN_ICANN,
            // NN_PRIVATE
        };

        WindowingType windowing[] = {
            WINDOWING_Q,
            // WINDOWING_R,
            // WINDOWING_QR
        };

        double ninf[] = {
            -50, // -2E-22
        };

        double pinf[] = {
            50, // 2E-22
        };

        size_t wl_rank[] = {
            100,
            1000,
            // 10000,
            // 100000
        };

        double wl_value[] = {
            0,
            -20,
            -50,
            -100,
            -1000
        };

        float nx_epsilon_increment[] = {
            0,
            0.25
        };

        #define _COPY(A, T) psetgenerator->n_## A = sizeof(A) / sizeof(T); psetgenerator->A = calloc(1, sizeof(A)); memcpy(psetgenerator->A, A, sizeof(A));

        _COPY(ninf, double);
        _COPY(pinf, double);
        _COPY(nn, NN);
        _COPY(windowing, WindowingType);
        _COPY(wl_rank, size_t);
        _COPY(wl_value, double);
        _COPY(nx_epsilon_increment, float);

        #undef _COPY
    }

    MANY(PSet) psets = parameters_generate(psetgenerator);

    parameters_generate_free(psetgenerator);

    if (subset && subset < psets.number) {
        psets.number = subset;
    }

    return psets;
}

MANY(Performance) make_performance() {
    MANY(Performance) performances;

    INITMANY(performances, 10, Performance);

    int i = 0;
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_1];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_05];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_01];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_TPR2];
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_FPR];

    performances.number = i;

    return performances;
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

    // test_loadANDsave();
    // test_addpsets();

    // logger_initConsoleLogger(stdout);

#ifdef IO_DEBUG
    __io__debug = 0;
#endif

    logger_initFileLogger("log/log.txt", 1024 * 1024, 5);
    logger_setLevel(LogLevel_TRACE);
    logger_autoFlush(100);
    LOG_INFO("console logging");

    
    char rootdir[PATH_MAX];

    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/princio/Desktop/results/4_tests_45/");
    }

    char fpathtb2[200];
    char fpathcsv[200];
    char dirtrainer[200];

    RTestBed2 tb2;
    MANY(Performance) performances;
    RTrainer trainer;
    MANY(PSet) psets;
    MANY(WSize) wsizes;
    const size_t n_try = 10;

    tb2 = NULL;
    wsizes = make_wsizes();
    psets = make_parameters(0);
    performances = make_performance();

    snprintf(fpathtb2, PATH_MAX, "tb2_%ld_%ld.bin", wsizes._[0].value, psets.number);
    snprintf(fpathcsv, PATH_MAX, "tb2_%ld_%ld.csv", wsizes._[0].value, psets.number);
    io_path_concat(rootdir, fpathtb2, fpathtb2);
    io_path_concat(rootdir, fpathcsv, fpathcsv);
    io_path_concat(rootdir, "trainer/", dirtrainer);

    if (io_makedirs(rootdir)) {
        printf("Impossible to create directories: %s\n", rootdir);
        return -1;
    }

    if (io_makedirs(dirtrainer)) {
        printf("Impossible to create directories: %s\n", rootdir);
        return -1;
    }

    if (testbed2_io(IO_READ, fpathtb2, &tb2)) {
        {
            printf("Performing windowing...\n");
            tb2 = testbed2_create(wsizes, n_try);
            stratosphere_add(tb2, 45);
            testbed2_windowing(tb2);
        }
        {
            printf("Performing folding...\n");
            FoldConfig foldconfig;

            foldconfig.k = 10; foldconfig.k_test = 5;
            testbed2_fold_add(tb2, foldconfig);

            foldconfig.k = 10; foldconfig.k_test = 8;
            testbed2_fold_add(tb2, foldconfig);

            foldconfig.k = 20; foldconfig.k_test = 10;
            testbed2_fold_add(tb2, foldconfig);

            foldconfig.k = 20; foldconfig.k_test = 15;
            testbed2_fold_add(tb2, foldconfig);
        }
        {
            printf("Appling...\n");
            testbed2_addpsets(tb2, psets);
            testbed2_apply(tb2);
        }

        testbed2_io(IO_WRITE, fpathtb2, &tb2);
    }

    if (1) {
        printf("Start training.\n");
        trainer = trainer_run(tb2, performances, dirtrainer);
        // print_trainer(trainer);
        csv_trainer(fpathcsv, trainer);

        Stat stat = stat_run(trainer);

        stat_free(stat);
        trainer_free(trainer);
    }
    testbed2_free(tb2);
    gatherer_free_all();
    FREEMANY(performances);
    FREEMANY(wsizes);
    FREEMANY(psets);
    logger_close();

    return 0;
}