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


#define TR(CM, CL) ((double) (CM)[CL][1]) / ((CM)[CL][0] + (CM)[CL][1])

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
    TrainerResults* results = &trainer->results;

    RTestBed2 tb2 = trainer->kfold0->testbed2;
    RKFold0 kfold0 = trainer->kfold0;

    for (size_t w = 0; w < tb2->wsizes.number; w++) {
        for (size_t a = 0; a < tb2->psets.number; a++) {
            for (size_t k = 0; k < trainer->kfold0->config.kfolds; k++) {
                for (size_t t = 0; t < trainer->thchoosers.number; t++) {

                    Result* result = &RESULT_IDX((*results), w, a, t, k);

                    {
                        char headers[4][50];
                        char header[210];
                        size_t h_idx = 0;

                        sprintf(headers[h_idx++], "%5ld", tb2->wsizes._[w].value);
                        sprintf(headers[h_idx++], "%3ld", a);
                        sprintf(headers[h_idx++], "%3ld", k);
                        sprintf(headers[h_idx++], "%12s", trainer->thchoosers._[t].name);
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
}

MANY(WSize) make_wsizes() {
    MANY(WSize) wsizes;
    size_t index;
    size_t n;

    index = 0;

    WSize wsizes_arr[] = {
        {.index = index++, .value = 100},
        {.index = index++, .value = 500},
    };

    n = sizeof(wsizes_arr)/sizeof(WSize);
    INITMANY(wsizes, n, WSize);
    memcpy(wsizes._, wsizes_arr, n * sizeof(WSize));

    return wsizes;
}

MANY(PSet) make_parameters() {
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

    MANY(PSet) psets = parameters_generate(psetgenerator);

    parameters_generate_free(psetgenerator);

    return psets;
}

MANY(Performance) make_performance() {
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

RTestBed2 make_testbed() {
    RTestBed2 tb2;

    MANY(WSize) wsizes = make_wsizes();

    tb2 = testbed2_create(wsizes);

    stratosphere_add(tb2);

    testbed2_windowing(tb2);

    FREEMANY(wsizes);

    return tb2;
}

RTestBed2 run_testbed(IOReadWrite rw, char dirname[200]) {
    RTestBed2 tb2;

    if (rw == IO_WRITE) {
        MANY(PSet) psets;

        tb2 = make_testbed();

        psets = make_parameters();

        testbed2_apply(tb2, psets);

        FREEMANY(psets);
    }

    testbed2_io(rw, dirname, &tb2);

    return tb2;
}

RKFold0 run_kfold0(IOReadWrite rw, char dirname[200], RTestBed2 tb2) {
    RKFold0 kfold0;

    if (rw == IO_WRITE) {
        KFoldConfig0  kconfig0;

        kconfig0.balance_method = KFOLD_BM_NOT_INFECTED;
        kconfig0.kfolds = 5;
        kconfig0.test_folds = 1;
        kconfig0.split_method = KFOLD_SM_MERGE_12;
        kconfig0.shuffle = 1;
    
        kfold0 = kfold0_run(tb2, kconfig0);
    }

    kfold0_io(rw, dirname, tb2, &kfold0);

    return kfold0;
}

RTrainer run_trainer(IOReadWrite rw, char dirname[200], RKFold0 kfold0) {
    RTrainer trainer;

    if (rw == IO_WRITE) {
        MANY(Performance) performances;

        performances = make_performance();

        trainer = trainer_run(kfold0, performances);

        FREEMANY(performances);
    }

    trainer_io(rw, dirname, kfold0, &trainer);

    return trainer;
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

    char dirname[200];
    sprintf(dirname, "/home/princio/Desktop/results/test_2");
    io_makedir(dirname, 200, 0);

    IOReadWrite rws[2] = { IO_WRITE, IO_READ };

    for (size_t rw = 0; rw < 2; rw++) {
        RTestBed2 tb2;
        RKFold0 kfold0;
        RTrainer trainer;

        tb2 = run_testbed(rws[rw], dirname);
        kfold0 = run_kfold0(rws[rw], dirname, tb2);
        trainer = run_trainer(rws[rw], dirname, kfold0);

        print_trainer(trainer);

        windows_free();
        window0s_free();
        windowings_free();
        sources_free();
        datasets0_free();
        testbed2_free(tb2);
        kfold0_free(kfold0);
        trainer_free(trainer);
    }

    return 0;
}