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
#include "configsuite.h"
#include "colors.h"
#include "gatherer.h"
#include "io.h"
#include "logger.h"
#include "main_windowing.h"

// typedef struct Score {
//     double th;
//     double score;
//     Performance performance;
//     Detection fulldetection[N_DGACLASSES];
// } Score;

// MANY(Performance) performances;


ParameterGenerator main_parametergenerator() {
    ParameterGenerator pg;

    #define __SET(NAME_LW) \
        pg.NAME_LW ## _n = sizeof(NAME_LW) / sizeof(NAME_LW ## _t);\
        for(size_t i = 0; i < pg.NAME_LW ## _n; i++) { pg.NAME_LW[i] = NAME_LW[i]; }

    {
        ninf_t ninf[] = {
            0,
            // -10,
            // -25,
            // -50,
            // -100,
            // -150
        };
        __SET(ninf);
    }

    {
        pinf_t pinf[] = {
            0,
            // 10,
            // 25,
            // 50,
            // 100,
            // 150
        };
        __SET(pinf);
    }

    {
        nn_t nn[] = {
            NN_NONE,
            // NN_TLD,
            // NN_ICANN,
            // NN_PRIVATE
        };
        __SET(nn);
    }

    {
        wl_rank_t wl_rank[] = {
            0,
            // 100,
            // 1000,
            10000,
            // 100000
        };
        __SET(wl_rank);
    }

    {
        wl_value_t wl_value[] = {
            0,
            // -10,
            // -25,
            -50,
            // -100,
            // -150
        };
        __SET(wl_value);
    }

    {
        windowing_t windowing[] = {
            WINDOWING_Q,
            // WINDOWING_R,
            // WINDOWING_QR
        };
        __SET(windowing);
    }

    {
        nx_epsilon_increment_t nx_epsilon_increment[] = {
            0,
            // 0.05,
            // 0.1,
            // 0.25,
            // 0.5
        };
        __SET(nx_epsilon_increment);
    }

    #undef __SET

    return pg;
}

void make_parameters_toignore(ConfigSuite* cs) {
    {
        size_t idx = 0;
        cs->pr[PE_NINF]._[idx++].disabled = 0; // 0
        cs->pr[PE_NINF]._[idx++].disabled = 0; // -10
        cs->pr[PE_NINF]._[idx++].disabled = 0; // -25
        cs->pr[PE_NINF]._[idx++].disabled = 0; // -50
        cs->pr[PE_NINF]._[idx++].disabled = 0; // -100
        cs->pr[PE_NINF]._[idx++].disabled = 0; // -150
    }
    {
        size_t idx = 0;
        cs->pr[PE_PINF]._[idx++].disabled = 0; // 0
        cs->pr[PE_PINF]._[idx++].disabled = 0; // 10
        cs->pr[PE_PINF]._[idx++].disabled = 0; // 25
        cs->pr[PE_PINF]._[idx++].disabled = 0; // 50
        cs->pr[PE_PINF]._[idx++].disabled = 0; // 100
        cs->pr[PE_PINF]._[idx++].disabled = 0; // 150
    }
    {
        size_t idx = 0;
        cs->pr[PE_NN]._[idx++].disabled = 0; // NN_NONE
        cs->pr[PE_NN]._[idx++].disabled = 0; // NN_TLD
        cs->pr[PE_NN]._[idx++].disabled = 0; // NN_ICANN
        cs->pr[PE_NN]._[idx++].disabled = 0; // NN_PRIVATE
    }
    {
        size_t idx = 0;
        cs->pr[PE_WL_RANK]._[idx++].disabled = 0; // 0
        cs->pr[PE_WL_RANK]._[idx++].disabled = 0; // 100
        cs->pr[PE_WL_RANK]._[idx++].disabled = 0; // 1000
        cs->pr[PE_WL_RANK]._[idx++].disabled = 0; // 10000
        cs->pr[PE_WL_RANK]._[idx++].disabled = 0; // 100000
    }
    {
        size_t idx = 0;
        cs->pr[PE_WL_VALUE]._[idx++].disabled = 0; // 0
        cs->pr[PE_WL_VALUE]._[idx++].disabled = 0; // -10
        cs->pr[PE_WL_VALUE]._[idx++].disabled = 0; // -25
        cs->pr[PE_WL_VALUE]._[idx++].disabled = 0; // -50
        cs->pr[PE_WL_VALUE]._[idx++].disabled = 0; // -100
        cs->pr[PE_WL_VALUE]._[idx++].disabled = 0; // -150
    }
    {
        size_t idx = 0;
        cs->pr[PE_WINDOWING]._[idx++].disabled = 0; // WINDOWING_Q
        cs->pr[PE_WINDOWING]._[idx++].disabled = 0; // WINDOWING_R
        cs->pr[PE_WINDOWING]._[idx++].disabled = 0; // WINDOWING_QR
    }
    {
        size_t idx = 0;
        cs->pr[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0
        cs->pr[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.05
        cs->pr[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.1
        cs->pr[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.25
        cs->pr[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.5
    }
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

#ifdef LOGGING
    logger_initFileLogger("log/log.txt", 1024 * 1024, 5);
    logger_setLevel(LogLevel_DEBUG);
    logger_autoFlush(5000);
#endif

    
    char rootdir[PATH_MAX];

    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/princio/Desktop/results/test_configsuite/");
    }

    main_windowing_generate(rootdir, 2000, 20, main_parametergenerator());
    
    gatherer_free_all();

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}