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
// #include "experiment.h"
// #include "graph.h"
#include "parameters.h"
// #include "persister.h"
#include "stratosphere.h"
// #include "tester.h"
// #include "windowing.h"

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

int trys;
char try_name[5];


void exps_1() {
    WSizes wsizes;
    PSetGenerator psetgenerator;

    {
        wsizes.number = 2;
        wsizes._ = calloc(wsizes.number, sizeof(WSize));

        wsizes._[0].id = 0;
        wsizes._[0].value = 100;
        
        wsizes._[1].id = 1;
        wsizes._[1].value = 2500;
    //     wsizes._[1] = 50;
    //     wsizes._[2] = 100;
    //     wsizes._[3] = 200;
    //     wsizes._[4] = 2500;
    }
    
    {
        NN nn[] = { NN_NONE };//, NN_TLD, NN_ICANN, NN_PRIVATE };

        Whitelisting whitelisting[] = {
            { .rank = 0, .value = 0 },
            // { .rank = 1000, .value = -50 },
            // { .rank = 100000, .value = -50 },
            // { .rank = 1000000, .value = -10 },  
            // { .rank = 1000000, .value = -50 }
        };

        WindowingType windowing[] = {
            WINDOWING_Q,
            // WINDOWING_R,
            // WINDOWING_QR
        };

        InfiniteValues infinitevalues[] = {
            { .ninf = -20, .pinf = 20 }
        };

        psetgenerator.n_nn = sizeof(nn) / sizeof(nn);
        psetgenerator.nn = nn;

        psetgenerator.n_whitelisting = sizeof(whitelisting) / sizeof(Whitelisting);
        psetgenerator.whitelisting = whitelisting;

        psetgenerator.n_windowing = sizeof(windowing) / sizeof(WindowingType);
        psetgenerator.windowing = windowing;

        psetgenerator.n_infinitevalues = sizeof(infinitevalues) / sizeof(InfiniteValues);
        psetgenerator.infinitevalues = infinitevalues;
    }

    PSets psets = parameters_generate(&psetgenerator);

    Dataset0s datasets;
    datasets.number = psets.number * wsizes.number;
    datasets._ = calloc(datasets.number, sizeof(Dataset0));

    int32_t d = 0;
    for (int32_t p = 0; p < psets.number; p++) {
        for (int32_t w = 0; w < wsizes.number; w++) {
            datasets._[d].windowing.pset = &psets._[p];
            datasets._[d].windowing.wsize = wsizes._[w];
            ++d;
        }
    }

    Experiment exp;

    strcpy(exp.name, "exp_new_2");
    strcpy(exp.rootpath, "/home/princio/Desktop/exps");

    exp.psets = psets;
    exp.wsizes = wsizes;
    exp.KFOLDs = 10;

    Sources stratosphere_sources;

    stratosphere_run(&exp, datasets, &stratosphere_sources);

    // for (int32_t i = 0; i < datasets.number; i++) {
    //     for (int32_t w = 0; w < datasets._[i].windows[1].number; w++) {
    //         Window window = datasets._[i].windows[1]._[w];
    //         printf("(%d, %d, %d, %d, %d, %g)\n", i, 1, w, window.source_id, window.window_id, window.logit);
    //     }
    // }

    // DATASETs ARE READY
    // TEST FOR EACH DATASET, WHICH MEAN:
    //   - split train test
    //   - choose with some method (f1score, fpr) the threshold within the train subset
    //   - test with the choosen threshold the train subset
    //   - analyze the confusion matricies obtained within the test subset


    for (int32_t d = 0; d < datasets.number; d++) {
        DatasetFoldsConfig dfold;

        dfold.kfolds = 5;
        dfold.balance_method = DSBM_NOT_INFECTED;
        dfold.split_method = DSM_IGNORE_1;
        dfold.test_folds = dfold.kfolds - 1;
        dfold.shuffle = 0;


        DatasetTests tests = dataset_foldstest(&datasets._[d], dfold);

        for (int k = 0; k < tests.number; k++) {
            printf("FOLD %d\n", k);
            for (int t = 0; t < N_THCHOICEMETHODs; t++) {
                printf("  %12s %10g\t", thcm_names[t], tests._[k].ths[t]);
                for (int cl = 0; cl < N_CLASSES; cl++) {
                    printf("%6d %6d\t", tests._[k].multi[t][cl].falses, tests._[k].multi[t][cl].trues);
                }
                printf("\n");
            }
        }

        free(tests._);
    }
    

    free(wsizes._);
    free(psets._);
    free(stratosphere_sources._);

    for (int32_t i = 0; i < datasets.number; i++) {
        for (int32_t cl = 0; cl < N_CLASSES; cl++) {
            free(datasets._[i].windows[cl]._);
        }
    }

    free(datasets._);
    

    // Sources sources;

    // Sources sources_by_type[1];

    // Sources sources_by_type[0] = stratosphere_get_sources();

    // int32_t n_sources = 0;
    // for (size_t i = 0; i < 1; i++) {
    //     for (size_t s = 0; s < sources_by_type->number; s++) {
    //         n_sources += sources_by_type[i].number;
    //     }
    // }

    // sources.number = n_sources;
    // sources._ = calloc(n_sources, sizeof(Source));

    // n_sources = 0;
    // for (size_t i = 0; i < 1; i++) {
    //     memcpy(&sources._[n_sources], sources_by_type[i]._, sources_by_type[i].number * sizeof(Source));
    //     n_sources += sources_by_type[i].number;
    // }
    

    // ExperimentSet es;
    // Experiment exp;

    // strcpy(exp.name, "exp_5");
    // strcpy(exp.rootpath, "/home/princio/Desktop/exps");

    // exp.psets = parameters_generate(&psetgenerator);
    // exp.KFOLDs = 10;
    // exp.wsizes = wsizes;

    // // es.windowing = experiment_run("/home/princio/Desktop/exps", "exp_5", sources, wsizes, &psetgenerator);

    // for (int32_t s = 0; s < sources.number; s++) {
    //     for (int32_t i = 0; i < wsizes.number; i++) {
    //         sources._[s].fetch(sources._[s], wsizes._[i]);
    //     }
    // }
    

    // es.KFOLDs = 5;
    // es.N_SPLITRATIOs = 2;
    // {
    //     int i = 0;
    //     es.split_percentages[i++] = 0.01;
    //     es.split_percentages[i++] = 0.5;
    //     // es.split_percentages[i++] = 0.1;
    //     // es.split_percentages[i++] = 0.2;
    //     // es.split_percentages[i++] = 0.25;
    //     // es.split_percentages[i++] = 0.3;
    //     // es.split_percentages[i++] = 0.4;
    //     // es.split_percentages[i++] = 0.5;
    //     // es.split_percentages[i++] = 0.6;
    //     // es.split_percentages[i++] = 0.7;
    // }

    // es.evmfs.number = 5;
    // es.evmfs._ = calloc(5, sizeof(EvaluationMetricFunction));
    // int i = 0;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_f1score_beta1;
    // sprintf(es.evmfs._[i].name, "f1score_1");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_f1score_beta05;
    // sprintf(es.evmfs._[i].name, "f1score_05");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_f1score_beta01;
    // sprintf(es.evmfs._[i].name, "f1score_01");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_fpr;
    // sprintf(es.evmfs._[i].name, "fpr");
    // ++i;
    
    // es.evmfs._[i].fnptr = (EvaluationMetricFunctionPtr) &evfn_tpr;
    // sprintf(es.evmfs._[i].name, "tpr");
    // ++i;

    // experiment_test(&es);
}

int ff(const int cursor[4], const int sizes[4], int avg) {
    const int ncursor = 4;
    int active[ncursor];
    int last = 0;

    for (int i = 0; i < ncursor; i++) active[i] = 0;

    active[0] = 1;
    for (int i = 1; i < ncursor; i++) {
        active[i] = avg & (1 << (i-1)) ? 1 : 0;
        last = active[i] ? i : last;
    }

    int c = 0;
    for (int l = 0; l < last; l++) {
        int row = 1;
        if (cursor[l] == 0) continue;
        if (active[l] == 0) continue;
        for (int ll = l + 1; ll <= last; ll++) {
            if (active[ll]) row *= sizes[ll];
        }
        c += row * cursor[l];
    }
    
    c += cursor[last];

    return c;
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

    exps_1();

    exit(0);

    int n0 = 3;
    int n1 = 1;
    int n2 = 5;
    int n4 = 237;

    int cm0[n0];
    int cm1[n0][n1];
    int cm2[n0][n2];
    int cm3[n0][n1][n2];
    int cm4[n0][n4];
    int cm5[n0][n1][n4];
    int cm6[n0][n2][n4];
    int cm7[n0][n1][n2][n4];

    for (int i0 = 0; i0 < n0; i0++) {
        for (int i1 = 0; i1 < n1; i1++) {
            for (int i2 = 0; i2 < n2; i2++) {
                for (int i4 = 0; i4 < n4; i4++) {
                    cm0[i0] = rand() % 1000;
                    cm1[i0][i1] = rand() % 1000;
                    cm2[i0][i2] = rand() % 1000;
                    cm3[i0][i1][i2] = rand() % 1000;
                    cm4[i0][i4] = rand() % 1000;
                    cm5[i0][i1][i4] = rand() % 1000;
                    cm6[i0][i2][i4] = rand() % 1000;
                    cm7[i0][i1][i2][i4] = rand() % 1000;
                }
            }
        }
    }
    
    const int sizes[4] = { n0, n1, n2, n4 };

    int cursor[4];

    int wrongs = 0;
    for (int i0 = 0; i0 < n0; i0++) {
        for (int i1 = 1; i1 < n1; i1++) {
            for (int i2 = 0; i2 < n2; i2++) {
                for (int i4 = 1; i4 < n4; i4++) {
                    cursor[0] = i0;
                    cursor[1] = i1;
                    cursor[2] = i2;
                    cursor[3] = i4;
                    int avg = 0;
                    {
                        int a = cm0[i0];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm0)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += !(a == v);
                    }
                    {
                        avg = 4;
                        int a = cm4[i0][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm4)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 2;
                        int a = cm2[i0][i2];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm2)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 6;
                        int a = cm6[i0][i2][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm6)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 1;
                        int a = cm1[i0][i1];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm1)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 5;
                        int a = cm5[i0][i1][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm5)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 3;
                        int a = cm3[i0][i1][i2];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm3)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }

                    {
                        avg = 7;
                        int a = cm7[i0][i1][i2][i4];
                        int c = ff(cursor, sizes, avg);
                        int v = ((int*)cm7)[c];
                        printf("%5d\t%d\t%5d\t%5d\t%d\n", avg, c, a, v, a == v);
                        wrongs += (a != v);
                    }
                }
            }
        }
    }


    printf("%d\n", wrongs);


    exit(0);


/*
    // int N_SPLITRATIOs = 10;

    // double split_percentages[10] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7 };
    // int split_nwindows[N_SPLITRATIOs][3];

    // int KFOLDs = 50;

    // memset(split_nwindows, 0, N_SPLITRATIOs * 3 * sizeof(int));

    // Dataset *dt = calloc(windowing->wsizes.number, sizeof(Dataset));
    // dataset_fill(windowing, dt);

    // ConfusionMatrix cm[N_SPLITRATIOs][windowing->wsizes.number][KFOLDs][windowing->psets.number];

    // for (int p = 0; p < N_SPLITRATIOs; ++p) {
    //     double split_percentage = split_percentages[p];
    //     for (int w = 0; w < windowing->wsizes.number; ++w) {
    //         const int32_t wsize = windowing->wsizes._[w];
    //         for (int k = 0; k < KFOLDs; ++k) {
    //             DatasetTrainTest dt_tt;
    //             dataset_traintestsplit(&dt[w], &dt_tt, split_percentage);

    //             double ths[windowing->psets.number];
    //             memset(ths, 0, sizeof(double) * windowing->psets.number);

    //             WSetRef* ws = &dt_tt.train[CLASS__NOT_INFECTED];
    //             for (int i = 0; i < ws->number; i++)
    //             {
    //                 Window* window = ws->_[i];
    //                 for (int m = 0; m < window->metrics.number; m++) {
    //                     double logit = window->metrics._[m].logit;
    //                     if (ths[m] < logit) ths[m] = logit;
    //                 }
    //             }

    //             dataset_traintestsplit_cm(wsize, &windowing->psets, &dt_tt, windowing->psets.number, ths, &cm[p][w][k][0]); 
    //         }
    //     }
    // }


    // {
    //     const int32_t N_WSIZEs = windowing->wsizes.number;
    //     const int32_t N_METRICs = windowing->psets.number;

    //     #define CMAVG_INIT(N, ID)  \
    //     cmavgs[ID]._ = (CM*) cms_ ## ID;\
    //     cmavgs[ID].separate_id = (ID);\
    //     cmavgs[ID].totals = (N);\
    //     memset(cms_ ## ID, 0, sizeof(CM) * (N));

    //     CMAVG cmavgs[8];

    //     CM cms_0[N_SPLITRATIOs];
    //     CMAVG_INIT(N_SPLITRATIOs, 0);

    //     CM cms_1[N_SPLITRATIOs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_METRICs, 1);

    //     CM cms_2[N_SPLITRATIOs][KFOLDs];
    //     CMAVG_INIT(N_SPLITRATIOs * KFOLDs, 2);

    //     CM cms_3[N_SPLITRATIOs][KFOLDs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * KFOLDs * N_METRICs, 3);

    //     CM cms_4[N_SPLITRATIOs][N_WSIZEs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs, 4);

    //     CM cms_5[N_SPLITRATIOs][N_WSIZEs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * N_METRICs, 5);

    //     CM cms_6[N_SPLITRATIOs][N_WSIZEs][KFOLDs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs, 6);

    //     CM cms_7[N_SPLITRATIOs][N_WSIZEs][KFOLDs][N_METRICs];
    //     CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs, 7);


    //     // ( SPLITs=0!, WSIZEs, KFOLDs, METRICs )
    //     // 1 means separating
    //     // 0: ( 1, 0, 0, 0 )   |   SPLIT                        | WSIZE * METRICs * KFOLD
    //     // 1: ( 1, 0, 0, 1 )   |   SPLIT, METRICs               | WSIZE * METRICs * KFOLD
    //     // 2: ( 1, 0, 1, 0 )   |   SPLIT, KFOLD                 |
    //     // 3: ( 1, 0, 1, 1 )   |   SPLIT, KFOLD, METRICs        |
    //     // 4: ( 1, 1, 0, 0 )   |   SPLIT, WSIZE                 |
    //     // 5: ( 1, 1, 0, 1 )   |   SPLIT, WSIZE, METRIC         |
    //     // 6: ( 1, 1, 1, 0 )   |   SPLIT, WSIZE, KFOLD          | 
    //     // 7: ( 1, 1, 1, 1 )   |   SPLIT, WSIZE, KFOLD, METRIC  | 
    //     for (int s = 0; s < N_SPLITRATIOs; ++s) {
    //         for (int w = 0; w < N_WSIZEs; ++w) {
    //             for (int k = 0; k < KFOLDs; ++k) {
    //                 for (int m = 0; m < N_METRICs; ++m) {
    //                     for (int cl = 0; cl < N_CLASSES; ++cl) {
    //                         int falses = cm[s][w][k][m].classes[cl][0];
    //                         int trues = cm[s][w][k][m].classes[cl][1];
    //                         { // 
    //                             cms_0[s].classes[cl][0] += falses;
    //                             cms_0[s].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_1[s][m].classes[cl][0] += falses;
    //                             cms_1[s][m].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_2[s][k].classes[cl][0] += falses;
    //                             cms_2[s][k].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_3[s][k][m].classes[cl][0] += falses;
    //                             cms_3[s][k][m].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_4[s][w].classes[cl][0] += falses;
    //                             cms_4[s][w].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_5[s][w][m].classes[cl][0] += falses;
    //                             cms_5[s][w][m].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_6[s][w][k].classes[cl][0] += falses;
    //                             cms_6[s][w][k].classes[cl][1] += trues;
    //                         }
    //                         { // 
    //                             cms_7[s][w][k][m].classes[cl][0] += falses;
    //                             cms_7[s][w][k][m].classes[cl][1] += trues;
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }

    //     const int32_t MAX = N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs;
    //     for (int i = 0; i < MAX; ++i) {
    //         for (int k = 0; k < 8; ++k) {
    //             for (int cl = 0; cl < N_CLASSES; ++cl) {
    //                 if (i < cmavgs[k].totals) {
    //                     cmavgs[k]._[i].classes[cl][0] /= cmavgs[k].totals;
    //                     cmavgs[k]._[i].classes[cl][1] /= cmavgs[k].totals;
    //                 }
    //             }
    //         }
    //     }
    // }

    return EXIT_SUCCESS;
    */
}