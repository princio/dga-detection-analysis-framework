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
#include "gatherer.h"
#include "io.h"
// #include "logger.h"
#include "main_dataset.h"
#include "main_windowing.h"
#include "main_training.h"
#include "parameter_generator.h"
#include "wqueue.h"
#include "performance_defaults.h"
#include "thrange.h"

#include <math.h>

enum BO {
    BO_LOAD_OR_GENERATE,
    BO_TEST
};

MANY(Performance) _main_training_performance() {
    MANY(Performance) performances;

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

    const int wsize = 400;
    const int nsources = 20;
    const size_t max_configs = 0;

    const ParameterGenerator pg = parametergenerator_default(max_configs);

    if (QM_MAX_SIZE < (wsize * 3)) {
        printf("Error: wsize too large\n");
        exit(0);
    }

    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/oem/Desktop/results/test1_%d_%d_%ld/", wsize, nsources, (max_configs ? max_configs : pg.max_size));
    }

    RTB2W tb2w = NULL;
    RTB2D tb2d = NULL;

    enum BO todo = BO_LOAD_OR_GENERATE;
    
    switch (todo) {
        case BO_LOAD_OR_GENERATE: {
            tb2w = main_windowing_load(rootdir);
            if (!tb2w) {
                tb2w = main_windowing_generate(rootdir, wsize, nsources, pg);
            }
            break;
        }
        case BO_TEST: {
            if (main_windowing_test(rootdir, wsize, nsources, pg)) {
                LOG_INFO("TB2W test failed.");
                exit(1);
            }
            tb2w = main_windowing_load(rootdir);
            break;
        }
        default:
            break;
    }
    
    if(!tb2w) {
        printf("Error: TB2W is NULL.");
        exit(1);
    }

    MANY(FoldConfig) foldconfigs_many; {
        FoldConfig foldconfigs[] = {
            { .k = 10, .k_test = 2 },
            // { .k = 10,.k_test = 4, },
            // { .k = 10,.k_test = 6, },
            // { .k = 10,.k_test = 8, },
        };

        MANY_INIT(foldconfigs_many, sizeof(foldconfigs) / sizeof(FoldConfig), FoldConfig);
        memcpy(foldconfigs_many._, foldconfigs, sizeof(foldconfigs));
    }

    switch (todo) {
        case BO_LOAD_OR_GENERATE: {
            tb2d = main_dataset_load(rootdir, tb2w);
            if (!tb2d) {
                tb2d = main_dataset_generate(rootdir, tb2w, 1, foldconfigs_many);
            }
            break;
        }
        case BO_TEST: {
            if (main_dataset_test(rootdir, tb2w, 10, foldconfigs_many)) {
                LOG_INFO("TB2D test failed.");
                exit(1);
            }
            tb2d = main_dataset_load(rootdir, tb2w);
            break;
        }
        default:
            break;
    }

    BY_FOR(*tb2d, try) {
        BY_FOR(*tb2d, fold) {
            for (size_t idxsplit = 0; idxsplit < BY_GET2(*tb2d, try, fold).splits.number; idxsplit++) {
                dataset_minmax(BY_GET2(*tb2d, try, fold).splits._[idxsplit].train);
                dataset_minmax(BY_GET2(*tb2d, try, fold).splits._[idxsplit].test);
            }
        }
    }

    if(!tb2d) {
        LOG_ERROR("TB2D is NULL.");
        exit(1);
    }

    MANY(Performance) thchoosers = _main_training_performance();

    // {
    //     ThRange* thrange;
    //     thrange_context* context = thrange_start(tb2d->tb2w, thchoosers);
    //     thrange = context->thrange;
    //     thrange_wait(context);
    //     thrange_free(context->thrange);
    // }

    RTrainer trainer = main_training_generate(rootdir, tb2d, thchoosers);

    main_training_stat(trainer);

    trainer_free(trainer);
    tb2d_free(tb2d);
    tb2w_free(tb2w);


    MANY_FREE(foldconfigs_many);
    MANY_FREE(thchoosers);

    gatherer_free_all();

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}
