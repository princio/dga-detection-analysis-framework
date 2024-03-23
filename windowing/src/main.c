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

    configsuite_generate(pg);

    stratosphere_add("CTU-SME-11", 0);

    windowing_apply(100);

    RWindowMC windowmc;
    MANY(RWindowing) windowingmany;

    windowingmany = windowing_many_get();

    windowmc = g2_insert_alloc_item(G2_WMC);
    windowmc_init(windowmc);
    windowmc_buildby_windowing_many(windowmc, windowingmany);
 
    WindowFoldConfig wfc = { .k = 10, .k_test = 5 };
    windowfold_create(windowmc, wfc);

    WindowSplitConfig config = { 
        .how = WINDOWSPLIT_HOW_BY_DAY,
        .day = 1
    };

    // RWindowSplit split1 = windowsplit_createby_day(windowingmany, 1);
    for (size_t i = 0; i < 10; i++) {
        RWindowSplit split = windowsplit_createby_portion(windowmc, 1, 10);
    }

    RWindowSplit split = windowsplit_createby_day(windowingmany, 0);

    MANY_FREE(windowingmany);

    g2_io_all(IO_WRITE);

    g2_free_all();

    g2_init();
    g2_io_all(IO_READ);
    g2_free_all();

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}
