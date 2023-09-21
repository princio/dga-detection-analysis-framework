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
    

    int N_SPLITRATIOs = 10;

    double split_percentages[10] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7 };
    int split_nwindows[N_SPLITRATIOs][3];

    int KFOLDs = 50;

    memset(split_nwindows, 0, N_SPLITRATIOs * 3 * sizeof(int));

    Dataset *dt = calloc(windowing.wsizes.number, sizeof(Dataset));
    dataset_fill(&windowing, dt);


    ConfusionMatrix cm[N_SPLITRATIOs][windowing.wsizes.number][KFOLDs][windowing.psets.number];

    for (int p = 0; p < N_SPLITRATIOs; ++p) {
        double split_percentage = split_percentages[p];
        for (int w = 0; w < windowing.wsizes.number; ++w) {
            const int32_t wsize = windowing.wsizes._[w];
            for (int k = 0; k < KFOLDs; ++k) {
                DatasetTrainTest dt_tt;
                dataset_traintest(&dt[w], &dt_tt, split_percentage);

                double ths[windowing.psets.number];
                memset(ths, 0, sizeof(double) * windowing.psets.number);

                WSetRef* ws = &dt_tt.train[CLASS__NOT_INFECTED];
                for (int i = 0; i < ws->number; i++)
                {
                    Window* window = ws->_[i];
                    for (int m = 0; m < window->metrics.number; m++) {
                        double logit = window->metrics._[m].logit;
                        if (ths[m] < logit) ths[m] = logit;
                    }
                }

                dataset_traintest_cm(wsize, &windowing.psets, &dt_tt, windowing.psets.number, ths, &cm[p][w][k][0]); 
            }
        }
    }


    {
        const int32_t N_WSIZEs = windowing.wsizes.number;
        const int32_t N_METRICs = windowing.psets.number;

        #define CMAVG_INIT(N, ID)  \
        cmavgs[ID]._ = (CM*) cms_ ## ID;\
        cmavgs[ID].separate_id = (ID);\
        cmavgs[ID].totals = (N);\
        memset(cms_ ## ID, 0, sizeof(CM) * (N));

        CMAVG cmavgs[8];

        CM cms_0[N_SPLITRATIOs];
        CMAVG_INIT(N_SPLITRATIOs, 0);

        CM cms_1[N_SPLITRATIOs][N_METRICs];
        CMAVG_INIT(N_SPLITRATIOs * N_METRICs, 1);

        CM cms_2[N_SPLITRATIOs][KFOLDs];
        CMAVG_INIT(N_SPLITRATIOs * KFOLDs, 2);

        CM cms_3[N_SPLITRATIOs][KFOLDs][N_METRICs];
        CMAVG_INIT(N_SPLITRATIOs * KFOLDs * N_METRICs, 3);

        CM cms_4[N_SPLITRATIOs][N_WSIZEs];
        CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs, 4);

        CM cms_5[N_SPLITRATIOs][N_WSIZEs][N_METRICs];
        CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * N_METRICs, 5);

        CM cms_6[N_SPLITRATIOs][N_WSIZEs][KFOLDs];
        CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs, 6);

        CM cms_7[N_SPLITRATIOs][N_WSIZEs][KFOLDs][N_METRICs];
        CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs, 7);


        // ( SPLITs=0!, WSIZEs, KFOLDs, METRICs )
        // 1 means separating
        // 0: ( 1, 0, 0, 0 )   |   SPLIT                        | WSIZE * METRICs * KFOLD
        // 1: ( 1, 0, 0, 1 )   |   SPLIT, METRICs               | WSIZE * METRICs * KFOLD
        // 2: ( 1, 0, 1, 0 )   |   SPLIT, KFOLD                 |
        // 3: ( 1, 0, 1, 1 )   |   SPLIT, KFOLD, METRICs        |
        // 4: ( 1, 1, 0, 0 )   |   SPLIT, WSIZE                 |
        // 5: ( 1, 1, 0, 1 )   |   SPLIT, WSIZE, METRIC         |
        // 6: ( 1, 1, 1, 0 )   |   SPLIT, WSIZE, KFOLD          | 
        // 7: ( 1, 1, 1, 1 )   |   SPLIT, WSIZE, KFOLD, METRIC  | 
        for (int s = 0; s < N_SPLITRATIOs; ++s) {
            for (int w = 0; w < N_WSIZEs; ++w) {
                for (int k = 0; k < KFOLDs; ++k) {
                    for (int m = 0; m < N_METRICs; ++m) {
                        for (int cl = 0; cl < N_CLASSES; ++cl) {
                            int falses = cm[s][w][k][m].classes[cl][0];
                            int trues = cm[s][w][k][m].classes[cl][1];
                            { // 
                                cms_0[s].classes[cl][0] += falses;
                                cms_0[s].classes[cl][1] += trues;
                            }
                            { // 
                                cms_1[s][m].classes[cl][0] += falses;
                                cms_1[s][m].classes[cl][1] += trues;
                            }
                            { // 
                                cms_2[s][k].classes[cl][0] += falses;
                                cms_2[s][k].classes[cl][1] += trues;
                            }
                            { // 
                                cms_3[s][k][m].classes[cl][0] += falses;
                                cms_3[s][k][m].classes[cl][1] += trues;
                            }
                            { // 
                                cms_4[s][w].classes[cl][0] += falses;
                                cms_4[s][w].classes[cl][1] += trues;
                            }
                            { // 
                                cms_5[s][w][m].classes[cl][0] += falses;
                                cms_5[s][w][m].classes[cl][1] += trues;
                            }
                            { // 
                                cms_6[s][w][k].classes[cl][0] += falses;
                                cms_6[s][w][k].classes[cl][1] += trues;
                            }
                            { // 
                                cms_7[s][w][k][m].classes[cl][0] += falses;
                                cms_7[s][w][k][m].classes[cl][1] += trues;
                            }
                        }
                    }
                }
            }
        }

        const int32_t MAX = N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs;
        for (int i = 0; i < MAX; ++i) {
            for (int k = 0; k < 8; ++k) {
                for (int cl = 0; cl < N_CLASSES; ++cl) {
                    if (i < cmavgs[k].totals) {
                        cmavgs[k]._[i].classes[cl][0] /= cmavgs[k].totals;
                        cmavgs[k]._[i].classes[cl][1] /= cmavgs[k].totals;
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}