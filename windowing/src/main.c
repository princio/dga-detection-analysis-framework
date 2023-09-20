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
#include "dataset.h"
#include "dn.h"
#include "parameters.h"
#include "persister.h"
#include "stratosphere.h"
#include "windowing.h"

#include <stdint.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* for ntohl/htonl */
#include <netinet/in.h>
#include <arpa/inet.h>

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


    // persister_test();

    // exit(0);

    char root_dir[100];
    sprintf(root_dir, "/home/princio/Desktop/exps/exp_0920");
    struct stat st = {0};
    if (stat(root_dir, &st) == -1) {
        mkdir(root_dir, 0700);
    }

    Windowing windowing;

    windowing.wsizes.number = 5;
    windowing.wsizes._[0] = 10;
    windowing.wsizes._[1] = 50;
    windowing.wsizes._[2] = 100;
    windowing.wsizes._[3] = 200;
    windowing.wsizes._[4] = 2500;

    parameters_generate(&windowing);

    stratosphere_add_captures(&windowing);

    windowing_fetch(&windowing);
    

    int N_SPLITS = 10;

    double split_percentages[10] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7 };
    int split_nwindows[N_SPLITS][3];

    int KFOLD = 50;

    memset(split_nwindows, 0, N_SPLITS * 3 * sizeof(int));

    Dataset dt[windowing.wsizes.number];
    dataset_fill(&windowing, dt);


    int32_t cm[windowing.wsizes.number][N_SPLITS][KFOLD][windowing.psets.number][2];

    for (int w = 0; w < windowing.wsizes.number; ++w) {
        for (int p = 0; p < N_SPLITS; ++p) {
            double split_percentage = split_percentages[p];
            for (int k = 0; k < KFOLD; ++k) {
                DatasetTrainTest dt_tt;
                dataset_traintest(&dt[w], &dt_tt, split_percentage);

                double ths[windowing.psets.number];
                memset(ths, 0, sizeof(double) * windowing.psets.number);

                WSetPtr ws = &dt_tt.train[CLASS__NOT_INFECTED];
                for (int i = 0; i < ws->n_windows; i++)
                {
                    Window* window = &ws->windows[i];
                    for (int m = 0; m < window->metrics.number; m++) {
                        double logit = window->metrics._[m].logit;
                        if (ths[m] < logit) ths[m] = logit;
                    }
                }

                dataset_traintest_cm(&windowing.psets, &dt_tt, ths, cm[w][p][k]);
            }
        }
    }

    return EXIT_SUCCESS;
}