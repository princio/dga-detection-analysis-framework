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
#include "windowsplitdetection.h"

#include <assert.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

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
    logger_initFileLogger("log/log.txt", 1024 * 1024, 5);
    // logger_initConsoleLogger(stdout);
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
        sprintf(rootdir, "/home/princio/Desktop/results/dns3/sme-11/test_new_tld/"); //, wsize, nsources, (max_configs ? max_configs : pg.max_size));
    }

    io_setdir(rootdir);

    char dataset[20];
    strcpy(dataset, "CTU-SME-11");
    
    int compute = 1;
    
    g2_init();

    if (g2_io_call(G2_WING, IO_READ)) {
        configsuite_generate(pg);

        stratosphere_add(dataset, 0);

        windowing_apply(100);

        g2_io_call(G2_WING, IO_WRITE);
    }

if (compute) {
    RWindowMC windowmc;

    windowmc = g2_insert_alloc_item(G2_WMC);
    windowmc_init(windowmc);
    windowmc_buildby_windowing(windowmc);
 
    WindowFoldConfig wfc = { .k = 10, .k_test = 5 };
    windowfold_create(windowmc, wfc);

    WindowSplitConfig config = { 
        .how = WINDOWSPLIT_HOW_BY_DAY,
        .day = 1
    };

    
    if (strcmp(dataset, "CTU-SME-11") == 0) {
        windowsplit_createby_day(0);
    } else {
        RWindowMC windowmc;
        windowmc = (RWindowMC) g2_insert_alloc_item(G2_WMC);
        windowmc_init(windowmc);
        windowmc_buildby_windowing(windowmc);
        for (int k = 1; k <= 10; k++) {
            windowsplit_createby_portion(windowmc, k, 10);
        }
    }

    {
        __MANY many = g2_array(G2_W0MANY);
        for (size_t i = 0; i < many.number; i++) {
            RWindow0Many w0many = (RWindow0Many) many._[i];
            assert(w0many->number);
            assert(w0many->_[0].applies.number);
            assert(w0many->_[0].applies._);
            for (size_t w = 0; w < w0many->number; w++) {
                assert(w0many->_[w].n_message > 0);
            }
        }
    }
} else {
    g2_io_all(IO_READ);
}

    // size_t wmany_0 = 0;
    // size_t wmc[3];
    // size_t wsplit[4];
    // {
    //     __MANY many = g2_array(G2_WMANY);
    //     for (size_t i = 0; i < many.number; i++) {
    //         RWindowMany windowmany = (RWindowMany) many._[i];
    //         wmany_0 += windowmany->number == 0;
    //     }
    //     printf("%20s\t%ld\n", "All many's having 0 length:", wmany_0);
    // }
    // {
    //     memset(&wmc, 0, sizeof(wmc));
    //     __MANY many = g2_array(G2_WMC);
    //     for (size_t i = 0; i < many.number; i++) {
    //         RWindowMC windowmc = (RWindowMC) many._[i];

    //         wmc[0] += windowmc->all->number == 0;
    //         BINFOR(bc) wmc[1] += windowmc->binary[bc]->number == 0;
    //         DGAFOR(cl) wmc[2] += windowmc->multi[cl]->number == 0;
    //     }
    //     printf("%20s\t%ld\n", "All MC-all having 0 length:", wmc[0]);
    //     printf("%20s\t%ld\n", "All MC-bin having 0 length:", wmc[1]);
    //     printf("%20s\t%ld\n", "All MC-mul having 0 length:", wmc[2]);
    // }
    // {
    //     memset(&wsplit, 0, sizeof(wmc));
    //     __MANY many = g2_array(G2_WSPLIT);
    //     for (size_t i = 0; i < many.number; i++) {
    //         RWindowSplit windowsplit = (RWindowSplit) many._[i];

    //         wmc[0] += windowsplit->train->number == 0;
    //         wmc[1] += windowsplit->test->all->number == 0;
    //         BINFOR(bc) wmc[2] += windowsplit->test->binary[bc]->number == 0;
    //         DGAFOR(cl) wmc[3] += windowsplit->test->multi[cl]->number == 0;
    //     }
    //     printf("%20s\t%ld\n", "All SPLIT-train    having 0 length:", wsplit[0]);
    //     printf("%20s\t%ld\n", "All SPLIT-test-all having 0 length:", wsplit[1]);
    //     printf("%20s\t%ld\n", "All SPLIT-test-bin having 0 length:", wsplit[2]);
    //     printf("%20s\t%ld\n", "All SPLIT-test-mul having 0 length:", wsplit[3]);
    // }

    {
        FILE* fp = fopen("/tmp/csv.csv", "w");
        __MANY many;
        many = g2_array(G2_W0MANY);
        for (size_t i = 0; i < many.number; i++) {
            RWindow0Many window0many = (RWindow0Many) many._[i];
            IndexMC count;
            memset(&count, 0, sizeof(IndexMC));
            count.all = window0many->number;
            count.binary[window0many->windowing->source->wclass.bc] = window0many->number;
            count.multi[window0many->windowing->source->wclass.mc] = window0many->number;
            fprintf(fp, "w0many,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        many = g2_array(G2_WMANY);
        for (size_t i = 0; i < many.number; i++) {
            RWindowMany windowmany = (RWindowMany) many._[i];
            IndexMC count = windowmany_count(windowmany);
            fprintf(fp, "wmany,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        many = g2_array(G2_WMC);
        for (size_t i = 0; i < many.number; i++) {
            RWindowMC windowmc;
            IndexMC count;

            windowmc = (RWindowMC) many._[i];
        
            count = windowmc_count(windowmc);
            fprintf(fp, "windowmc,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        many = g2_array(G2_WSPLIT);
        for (size_t i = 0; i < many.number; i++) {
            RWindowSplit windowsplit;
            IndexMC count;

            windowsplit = (RWindowSplit) many._[i];
            count = windowmany_count(windowsplit->train);

            fprintf(fp, "split-train,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");

            count = windowmc_count(windowsplit->test);
            fprintf(fp, "split-test,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

    // {
    //     __MANY many = g2_array(G2_WSPLIT);
    //     for (size_t i = 0; i < many.number; i++) {
    //        windowsplit_minmax((RWindowSplit) many._[i]);
    //     }
    // }

    void* context = windowsplitdetection_start();
    windowsplitdetection_wait(context);

if (compute) {
    g2_io_all(IO_WRITE);
}

    g2_free_all();

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}
