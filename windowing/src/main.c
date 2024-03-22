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


#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "common.h"
#include "configsuite.h"
#include "colors.h"
#include "gatherer2.h"
#include "io.h"
// #include "logger.h"
#include "parameter_generator.h"
#include "wqueue.h"
#include "performance_defaults.h"
#include "source.h"
#include "stratosphere.h"
#include "windowing.h"
#include "windowmc.h"
#include "windowfold.h"
#include "windowsplit.h"

#include <math.h>

ConfigSuite configsuite;
MANY(Performance) windowing_thchooser;

enum BO {
    BO_LOAD_OR_GENERATE,
    BO_TEST
};

MANY(Performance) _main_training_performance() {
    MANY(Performance) performances;

    performances.number = 0;
    performances._ = NULL;

    MANY_INIT(performances, 10, Performance);

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

    // cwin = initscr();
    raw();

#ifdef IO_DEBUG
    __io__debug = 0;
#endif

#ifdef LOGGING
    // logger_initFileLogger("log/log.txt", 1024 * 1024, 5);
    logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_TRACE);
    logger_autoFlush(5000);
#endif

    char rootdir[PATH_MAX];

    const int wsize = 100;
    const int nsources = 0;
    const size_t max_configs = 10;
    const size_t n_try = 1;

    const ParameterGenerator pg = parametergenerator_default(max_configs);

    if (QM_MAX_SIZE < (wsize * 3)) {
        printf("Error: wsize too large\n");
        exit(0);
    }

    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/princio/Desktop/results/dns3/test3_%d_%d_%ld/", wsize, nsources, (max_configs ? max_configs : pg.max_size));
    }

    io_setdir(rootdir);
    g2_init();

    MANY(__Source) source_write;
    MANY(__Windowing) windowing_write;
    source_write.number = 0;
    source_write._ = 0;
    windowing_write.number = 0;
    windowing_write._ = 0;

    configsuite_generate(&configsuite, pg);
    stratosphere_add("CTU-SME-11", 0);

    {
        __MANY many = g2_array(G2_SOURCE);
        MANY_INIT(source_write, many.number, __Source);
        for (size_t i = 0; i < many.number; i++) {
            RSource source = many._[i];
            memcpy(&source_write._[i], source, sizeof(__Source));
        }
    }
    
    windowing_apply(500);

    // RWindowMC windowmc;
    // MANY(RWindowing) windowingmany;

    // windowingmany = windowing_many_get();

    // windowmc = g2_insert_alloc_item(G2_WMC);

    // windowmc_init(windowmc);

    // windowmc_buildby_windowing_many(windowmc, windowingmany);
 
    // WindowFoldConfig config = { .k = 10, .k_test = 5 };
    // windowfold_create(windowmc, config);

    {
        __MANY many = g2_array(G2_WING);
        MANY_INIT(windowing_write, many.number, __Source);
        for (size_t i = 0; i < many.number; i++) {
            RWindowing windowing = many._[i];
            memcpy(&windowing_write._[i], windowing, sizeof(__Source));
        }
    }

    g2_io_all(IO_WRITE);
    g2_free_all();

    g2_init();
    g2_io_all(IO_READ);

    {
        __MANY many = g2_array(G2_SOURCE);
        int errors = 0;
        for (size_t i = 0; i < many.number; i++) {
            RSource source = many._[i];
            int mismatch = memcmp(&source_write._[i], source, sizeof(__Source));
            if (mismatch) {
                printf("SOURCE %ld mismatch: %d\n", i, mismatch);
            }
        }
        printf("SOURCE errors: %d\n", errors);
    }
    
    {
        __MANY many = g2_array(G2_WING);
        int errors = 0;
        for (size_t i = 0; i < many.number; i++) {
            RWindowing windowing = many._[i];
            int mismatch = 0;
            
            mismatch += windowing_write._[i].g2index != windowing->g2index;
            mismatch += windowing_write._[i].wsize != windowing->wsize;
            mismatch += strcmp(windowing_write._[i].source->name, windowing->source->name);
            mismatch += windowing_write._[i].windowmany->number != windowing->windowmany->number;
            mismatch += windowing_write._[i].windowmany->g2index != windowing->windowmany->g2index;

            if (windowing_write._[i].windowmany->g2index != windowing->windowmany->g2index)
            printf("g2index: %ld != %ld\n",  windowing_write._[i].windowmany->g2index, windowing->windowmany->g2index);

            if (mismatch) {
                printf("WINDOWING %ld mismatch: %d\n", i, mismatch);
            }

            errors += mismatch > 0;
        }
        printf("WINDOWING errors: %d\n", errors);
    }


    g2_free_all();

    configset_free(&configsuite);

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}
