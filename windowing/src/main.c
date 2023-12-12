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
#include "fold.h"
#include "gatherer.h"
#include "io.h"
#include "parameters.h"
#include "performance_defaults.h"
#include "stratosphere.h"
#include "testbed2.h"
#include "trainer.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <limits.h>
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



#define TR(CM, CL) ((double) (CM)[CL][1]) / ((CM)[CL][0] + (CM)[CL][1])
void print_trainer(RTrainer trainer) {

    FILE* file_csv = fopen("results.csv", "w");

    TrainerResults* results = &trainer->results;

    RTestBed2 tb2 = trainer->tb2;


    fprintf(file_csv, ",,,,,,train,train,train,test,test,test");
    fprintf(file_csv, "fold,try,k,wsize,apply,thchooser,0,1,2,0,1,2");

    for (size_t w = 0; w < results->bywsize.number; w++) {
        for (size_t a = 0; a < results->bywsize._[w].byapply.number; a++) {
            for (size_t f = 0; f < results->bywsize._[w].byapply._[a].byfold.number; f++) {
                for (size_t try = 0; try < results->bywsize._[w].byapply._[a].byfold._[f].bytry.number; try++) {
                    for (size_t k = 0; k < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits.number; k++) {
                        for (size_t ev = 0; ev < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits._[k].bythchooser.number; ev++) {

                            Result* result = &RESULT_IDX((*results), w, a, f, try, k, ev);

                            {
                                char headers[4][50];
                                char header[210];
                                size_t h_idx = 0;

                                sprintf(headers[h_idx++], "%3ld", f);
                                sprintf(headers[h_idx++], "%3ld", try);
                                sprintf(headers[h_idx++], "%3ld", k);
                                sprintf(headers[h_idx++], "%5ld", tb2->wsizes._[w].value);
                                sprintf(headers[h_idx++], "%3ld", a);
                                sprintf(headers[h_idx++], "%12s", trainer->thchoosers._[ev].name);
                                sprintf(header, "%s,%s,%s,%s,", headers[0], headers[1], headers[2], headers[3]);
                                printf("%-30s ", header);
                            }

                            DGAFOR(cl) {
                                printf("%1.4f\t", TR(result->best_train.windows, cl));
                            }
                            printf("|\t");
                            DGAFOR(cl) {
                                printf("%1.4f\t", TR(result->best_test.windows, cl));
                            }
                            printf("\n");
                        }
                    }
                }
            }
        }
    }
}
#undef TR


#define TR(CM, CL) ((double) (CM)[CL][1]) / ((CM)[CL][0] + (CM)[CL][1])
void csv_trainer(char dirname[PATH_MAX], RTrainer trainer) {
    char fpath[PATH_MAX];
    FILE* file_csv;

    if (io_makedir(dirname, 0)) {
        printf("Error: impossible to create directory <%s>\n", dirname);
        return;
    }

    sprintf(fpath, "%s/results.csv", dirname);
    file_csv = fopen(fpath, "w");

    TrainerResults* results = &trainer->results;

    RTestBed2 tb2 = trainer->tb2;

    fprintf(file_csv, "caos,caos,caos,wsize,pset,pset,pset,pset,pset,pset,pset,pset,train,train,train,test,test,test\n");
    fprintf(file_csv, "fold,try,k,wsize,ninf,pinf,nn,windowing,wl_rank,wl_value,nx_eps_incr,thchooser,0,1,2,0,1,2\n");

    for (size_t w = 0; w < results->bywsize.number; w++) {
        for (size_t a = 0; a < results->bywsize._[w].byapply.number; a++) {
            for (size_t f = 0; f < results->bywsize._[w].byapply._[a].byfold.number; f++) {
                for (size_t try = 0; try < results->bywsize._[w].byapply._[a].byfold._[f].bytry.number; try++) {
                    for (size_t k = 0; k < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits.number; k++) {
                        for (size_t ev = 0; ev < results->bywsize._[w].byapply._[a].byfold._[f].bytry._[try].bysplits._[k].bythchooser.number; ev++) {
                            Result* result = &RESULT_IDX((*results), w, a, f, try, k, ev);

                            fprintf(file_csv, "%ld,", f);
                            fprintf(file_csv, "%ld,", try);
                            fprintf(file_csv, "%ld,", k);
                            fprintf(file_csv, "%ld,", tb2->wsizes._[w].value);
                            fprintf(file_csv, "%f,", tb2->applies._[a].pset.ninf);
                            fprintf(file_csv, "%f,", tb2->applies._[a].pset.pinf);
                            fprintf(file_csv, "%s,", NN_NAMES[tb2->applies._[a].pset.nn]);
                            fprintf(file_csv, "%s,", WINDOWING_NAMES[tb2->applies._[a].pset.windowing]);
                            fprintf(file_csv, "%ld,", tb2->applies._[a].pset.wl_rank);
                            fprintf(file_csv, "%f,", tb2->applies._[a].pset.wl_value);
                            fprintf(file_csv, "%f,", tb2->applies._[a].pset.nx_epsilon_increment);
                            fprintf(file_csv, "%s", trainer->thchoosers._[ev].name);

                            DGAFOR(cl) {
                                fprintf(file_csv, ",%1.4f", TR(result->best_train.windows, cl));
                            }
                            DGAFOR(cl) {
                                fprintf(file_csv, ",%1.4f", TR(result->best_test.windows, cl));
                            }
                            fprintf(file_csv, "\n");
                        }
                    }
                }
            }
        }
    }
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
            NN_TLD,
            NN_ICANN,
            NN_PRIVATE
        };

        WindowingType windowing[] = {
            WINDOWING_Q,
            // WINDOWING_R,
            // WINDOWING_QR
        };

        double ninf[] = {
            -50,
            // -20,
            0
        };

        double pinf[] = {
            50,
            // 20,
            0
        };

        size_t wl_rank[] = {
            // 100,
            1000,
            // 10000,
            100000
        };

        size_t wl_value[] = {
            0,
            // -20,
            -50,
            // -100
        };

        double nx_epsilon_increment[] = { 0.1 };

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
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_1];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_05];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_01];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_TPR];
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_FPR];

    performances.number = i;

    return performances;
}

RTrainer run_trainer(IOReadWrite rw, char dirname[200], RTestBed2 tb2) {
    RTrainer trainer;

    if (rw == IO_WRITE) {
        MANY(Performance) performances;

        performances = make_performance();

        trainer = trainer_run(tb2, performances);

        FREEMANY(performances);
    }

    trainer_io(rw, dirname, tb2, &trainer);

    return trainer;
}

void test_loadANDsave() {
    char dirname[200];

    for (size_t rw = 0; rw < 3; rw++) {
        RTestBed2 tb2;
        RTrainer trainer;

        tb2 = NULL;
        trainer = NULL;

        printf("------ RW %ld\n\n\n", rw);

        if (rw == 0) {
            {
                MANY(WSize) wsizes;
                wsizes = make_wsizes();
                tb2 = testbed2_create(wsizes);
                stratosphere_add(tb2);
                testbed2_windowing(tb2);
                FREEMANY(wsizes);
            }
            {
                FoldConfig foldconfig;
                foldconfig.tries = 1; foldconfig.k = 10; foldconfig.k_test = 8;
                fold_add(tb2, foldconfig);
                // foldconfig.tries = 5; foldconfig.k = 20; foldconfig.k_test = 15;
                // fold_add(tb2, foldconfig);
                // foldconfig.tries = 5; foldconfig.k = 10; foldconfig.k_test = 8;
                // fold_add(tb2, foldconfig);
            }

            sprintf(dirname, "/home/princio/Desktop/results/test/not_loaded/not_applied/");
            testbed2_io(IO_WRITE, dirname, &tb2);

            {
                MANY(PSet) psets;
                psets = make_parameters(2);
                testbed2_addpsets(tb2, psets);
                testbed2_apply(tb2);
                FREEMANY(psets);
            }

            sprintf(dirname, "/home/princio/Desktop/results/test/not_loaded/applied/");
            testbed2_io(IO_WRITE, dirname, &tb2);
        }

        if (rw == 1) {
            sprintf(dirname, "/home/princio/Desktop/results/test/not_loaded/not_applied/");
            testbed2_io(IO_READ, dirname, &tb2);
            sprintf(dirname, "/home/princio/Desktop/results/test/loaded/not_applied/");
            testbed2_io(IO_WRITE, dirname, &tb2);
            {
                MANY(PSet) psets;
                psets = make_parameters(2);
                testbed2_addpsets(tb2, psets);
                testbed2_apply(tb2);
                FREEMANY(psets);
            }
            sprintf(dirname, "/home/princio/Desktop/results/test/loaded/applied");
            testbed2_io(IO_WRITE, dirname, &tb2);
        } else
        if (rw == 2) {
            MANY(Performance) performances;

            sprintf(dirname, "/home/princio/Desktop/results/test/loaded/applied");
            testbed2_io(IO_READ, dirname, &tb2);
            performances = make_performance();
            trainer = trainer_run(tb2, performances);
            FREEMANY(performances);

            trainer_io(IO_WRITE, dirname, tb2, &trainer);
            print_trainer(trainer);
            trainer_free(trainer);
        }

        testbed2_free(tb2);
        gatherer_free_all();
    }
}

void test_addpsets() {
    char dirname[200];

    RTestBed2 tb2;
    MANY(Performance) performances;

    tb2 = NULL;
    performances = make_performance();

    sprintf(dirname, "/home/princio/Desktop/results/test/loaded/applied");
    testbed2_io(IO_READ, dirname, &tb2);

    {
        RTrainer trainer;
        trainer = trainer_run(tb2, performances);
        print_trainer(trainer);
        trainer_free(trainer);
    }

    {
        MANY(PSet) psets;
        psets = make_parameters(4);
        testbed2_addpsets(tb2, psets);
        FREEMANY(psets);
    }

    printf("--------\n");

    {
        RTrainer trainer;
        trainer = trainer_run(tb2, performances);
        print_trainer(trainer);
        trainer_free(trainer);
    }

    sprintf(dirname, "/home/princio/Desktop/results/test/loaded/addpsets");
    testbed2_io(IO_WRITE, dirname, &tb2);

    testbed2_free(tb2);
    gatherer_free_all();
    FREEMANY(performances);
}

void tb2_make(char fpath[PATH_MAX], MANY(WSize) wsizes, MANY(PSet) psets) {
    RTestBed2 tb2;
    RTrainer trainer;

    tb2 = NULL;
    trainer = NULL;

    {
        printf("Performing windowing...\n");
        tb2 = testbed2_create(wsizes);
        stratosphere_add(tb2);
        testbed2_windowing(tb2);
    }
    {
        printf("Performing folding...\n");
        FoldConfig foldconfig;
    
        foldconfig.tries = 2; foldconfig.k = 10; foldconfig.k_test = 2;
        fold_add(tb2, foldconfig);

        foldconfig.tries = 2; foldconfig.k = 10; foldconfig.k_test = 5;
        fold_add(tb2, foldconfig);

        foldconfig.tries = 2; foldconfig.k = 10; foldconfig.k_test = 8;
        fold_add(tb2, foldconfig);
    }
    {
        printf("Appling...\n");
        testbed2_addpsets(tb2, psets);
        testbed2_apply(tb2);
    }

    testbed2_io(IO_WRITE, fpath, &tb2);

    testbed2_free(tb2);
    gatherer_free_all();
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

    
    char rootdir[PATH_MAX - 100];

    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/princio/Desktop/results/0_tests/");
    }

    if (io_makedirs(rootdir)) {
        printf("Impossible to create directories: %s\n", rootdir);
        exit(1);
    }

    char fpathtb2[PATH_MAX];
    char fpathcsv[PATH_MAX];

    RTestBed2 tb2;
    MANY(Performance) performances;
    RTrainer trainer;
    MANY(PSet) psets;
    MANY(WSize) wsizes;

    tb2 = NULL;
    wsizes = make_wsizes();
    psets = make_parameters(0);
    performances = make_performance();
    snprintf(fpathtb2, PATH_MAX, "%s/tb2_%ld_%ld.bin", rootdir, wsizes._[0].value, psets.number);
    snprintf(fpathcsv, PATH_MAX, "%s/tb2_%ld_%ld.csv", rootdir, wsizes._[0].value, psets.number);

    // if (1) {
    if (testbed2_io(IO_READ, fpathtb2, &tb2)) {
        tb2_make(fpathtb2, wsizes, psets);
    }

    if (testbed2_io(IO_READ, fpathtb2, &tb2)) {
        printf("Impossible to continue.\n");
        return -1;
    }

    trainer = trainer_run(tb2, performances);
    // print_trainer(trainer);
    csv_trainer(fpathcsv, trainer);

    trainer_free(trainer);
    testbed2_free(tb2);
    gatherer_free_all();
    FREEMANY(performances);
    FREEMANY(wsizes);
    FREEMANY(psets);

    return 0;
}