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
#include "configsuite.h"
#include "colors.h"
#include "gatherer.h"
#include "io.h"
#include "logger.h"
#include "main_windowing.h"
#include "parameter_generator.h"
#include "wqueue.h"

// typedef struct Score {
//     double th;
//     double score;
//     Performance performance;
//     Detection fulldetection[N_DGACLASSES];
// } Score;

// MANY(Performance) performances;

// #define PARAMETERS_ALL
// #define PARAMETERS_MANY

int main (int argc, char* argv[]) {
    setbuf(stdout, NULL);

    // cwin = initscr();
    raw();

#ifdef IO_DEBUG
    __io__debug = 0;
#endif

#ifdef LOGGING
    logger_initFileLogger("log/log.txt", 1024 * 1024, 5);
    logger_setLevel(LogLevel_DEBUG);
    logger_autoFlush(5000);
#endif

    
    char rootdir[PATH_MAX];

    const int wsize = 2000;
    const int nsources = 0;
    const size_t max_configs = 10;

    if (QM_MAX_SIZE < (wsize * 3)) {
        printf("Error: wsize too large\n");
        exit(0);
    }

    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/princio/Desktop/results/test_configsuite_allsources_%d_%d_%ld/", wsize, nsources, max_configs);
    }

    if (0) {
        main_windowing_generate(rootdir, wsize, nsources, parametergenerator_default(max_configs));
        gatherer_free_all();

        RTB2W tb2w = main_windowing_load(rootdir);
        printf("%f\n", tb2w->windowing.bysource._[0]->windows._[0]->applies._[2].logit);
        tb2w_free(tb2w);
    }

    main_windowing_test(rootdir, wsize, nsources, parametergenerator_default(max_configs));

    gatherer_free_all();

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}