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
#include "common.h"
// #include "experiment.h"
#include "folding.h"
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


void source_count(Sources sources, int counts[N_DGACLASSES]) {
    memset(counts, 0, sizeof(int32_t));
    for (int32_t i = 0; i < sources.number; i++) {
        counts[sources._[i].dgaclass]++;
    }
}

void exps_1() {
    PSetGenerator psetgenerator;

    {
        int32_t wsizes[] = { 100 }; //, 5, 100 };//, 5, 100, 2500 };
    
    //     wsizes._[1] = 50;
    //     wsizes._[2] = 100;
    //     wsizes._[3] = 200;
    //     wsizes._[4] = 2500;

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


        psetgenerator.n_wsize = sizeof(wsizes) / sizeof(int32_t);
        psetgenerator.wsize = wsizes;

        psetgenerator.n_nn = sizeof(nn) / sizeof(NN);
        psetgenerator.nn = nn;

        psetgenerator.n_whitelisting = sizeof(whitelisting) / sizeof(Whitelisting);
        psetgenerator.whitelisting = whitelisting;

        psetgenerator.n_windowing = sizeof(windowing) / sizeof(WindowingType);
        psetgenerator.windowing = windowing;

        psetgenerator.n_infinitevalues = sizeof(infinitevalues) / sizeof(InfiniteValues);
        psetgenerator.infinitevalues = infinitevalues;
    }

    PSets psets = parameters_generate(&psetgenerator);

    Datasets datasets;
    datasets.number = psets.number;
    datasets._ = calloc(datasets.number, sizeof(Dataset));

    int32_t d = 0;
    for (int32_t p = 0; p < psets.number; p++) {
        datasets._[d].pset = &psets._[p];
        ++d;
    }

    Experiment exp;
    memset(&exp, 0, sizeof(Experiment));

    strcpy(exp.name, "exp_new_3");
    strcpy(exp.rootpath, "/home/princio/Desktop/exps");

    exp.psets = psets;

    Sources stratosphere_sources;

    stratosphere_run(&exp, datasets, &stratosphere_sources);

    experiment_sources_fill(&exp);

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

    printf("Sources loaded\n\n");

    /*
    for (int32_t d = 0; d < datasets.number; d++) {
        {
            printf("Processing dataset %d:\t", d);
            PSet* pset = datasets._[d].pset;
            printf("%8d, ", pset->wsize);
            printf("(%5g, %5g), ", pset->infinite_values.ninf, pset->infinite_values.pinf);
            printf("%6s, ", NN_NAMES[pset->nn]);
            printf("(%6d, %6g)\n", pset->whitelisting.rank, pset->whitelisting.value);
            // printf("%d\n", windowing->pset->windowing);
        }
        FoldingConfig dfold;

        dfold.kfolds = 5;
        dfold.balance_method = FOLDING_DSBM_NOT_INFECTED;
        dfold.split_method = FOLDING_DSM_IGNORE_1;
        dfold.test_folds = 1;
        dfold.shuffle = 0;

        folding_run(&exp, &datasets._[d], dfold);

        for (int k = 0; k < tests.number; k++) {
            printf("FOLD %2d  %15s\t", k, " ");
            for (int cl = 0; cl < N_DGACLASSES; cl++) {
                printf("%8d\t", tests._[k].multi[0][cl].all.falses + tests._[k].multi[0][cl].all.trues);
            }
            printf("\n");

            for (int t = 0; t < N_EVALUATEMETHODs; t++) {
                printf("  %12s %10g\t", thcm_names[t], tests._[k].ths[t]);
                for (int cl = 0; cl < N_DGACLASSES; cl++) {
                    printf("%8d\t", tests._[k].multi[t][cl].all.falses);
                }
                for (int cl = 0; cl < N_DGACLASSES; cl++) {
                    printf("%1.5f\t", (double) tests._[k].multi[t][cl].all.falses / (tests._[k].multi[t][cl].all.falses + tests._[k].multi[t][cl].all.trues));
                }
                printf("\n");
            }
        }

        DatasetFoldsDescribe dfb;
        memset(&dfb, 0, sizeof(DatasetFoldsDescribe));
        for (int t = 0; t < N_EVALUATEMETHODs; t++) {
            dfb.th[t].min = DBL_MAX;
            dfb.th[t].max = -1 * DBL_MAX;
            for (int cl = 0; cl < N_DGACLASSES; cl++) {
                dfb.falses[t][cl].min = DBL_MAX;
                dfb.trues[t][cl].min = DBL_MAX;
                dfb.false_ratio[t][cl].min = 1;
                dfb.true_ratio[t][cl].min = 1;
                dfb.sources_trues[t][cl].min = INT32_MAX;

                dfb.falses[t][cl].max = -1 * DBL_MAX;
                dfb.trues[t][cl].max = -1 * DBL_MAX;
                dfb.false_ratio[t][cl].max = 0;
                dfb.true_ratio[t][cl].max = 0;
                dfb.sources_trues[t][cl].max = 0;
            }
        }

        #define MIN(A, B) (A.min) = ((B) <= (A.min) ? (B) : (A.min))
        #define MAX(A, B) (A.max) = ((B) >= (A.max) ? (B) : (A.max))
        for (int k = 0; k < tests.number; k++) {
            Evaluation (*multi)[N_DGACLASSES] = tests._[k].multi;
            for (int t = 0; t < N_EVALUATEMETHODs; t++) {
                dfb.th[t].avg += tests._[k].ths[t];
                MIN(dfb.th[t], tests._[k].ths[t]);
                MAX(dfb.th[t], tests._[k].ths[t]);

                for (int cl = 0; cl < N_DGACLASSES; cl++) {
                    int32_t sources_trues = 0;
                    for (int32_t i = 0; i < exp.sources_arrays.multi[cl].number; i++) {
                        sources_trues += multi[t][cl].sources[i].trues > 0;
                    }
                    

                    dfb.false_ratio[t][cl].avg += FALSERATIO(multi[t][cl]);
                    dfb.true_ratio[t][cl].avg += TRUERATIO(multi[t][cl]);
                    dfb.falses[t][cl].avg += multi[t][cl].all.falses;
                    dfb.trues[t][cl].avg += multi[t][cl].all.trues;
                    dfb.sources_trues[t][cl].avg += sources_trues;

                    MIN(dfb.false_ratio[t][cl], FALSERATIO(multi[t][cl]));
                    MIN(dfb.true_ratio[t][cl], TRUERATIO(multi[t][cl]));
                    MIN(dfb.falses[t][cl], multi[t][cl].all.falses);
                    MIN(dfb.trues[t][cl], multi[t][cl].all.trues);
                    MIN(dfb.sources_trues[t][cl], sources_trues);

                    MAX(dfb.false_ratio[t][cl], FALSERATIO(multi[t][cl]));
                    MAX(dfb.true_ratio[t][cl], TRUERATIO(multi[t][cl]));
                    MAX(dfb.falses[t][cl], multi[t][cl].all.falses);
                    MAX(dfb.sources_trues[t][cl], sources_trues);
                }
            }
        }
        #undef MIN
        #undef MAX

        for (int t = 0; t < N_EVALUATEMETHODs; t++) {
            dfb.th[t].avg /= tests.number;
            for (int cl = 0; cl < N_DGACLASSES; cl++) {
                dfb.false_ratio[t][cl].avg /= tests.number;
                dfb.true_ratio[t][cl].avg /= tests.number;
                dfb.falses[t][cl].avg /= tests.number;
                dfb.trues[t][cl].avg /= tests.number;
                dfb.sources_trues[t][cl].avg /= tests.number;
            }
        }

        for (int t = 0; t < N_EVALUATEMETHODs; t++) {
            printf("\n%8s\n", thcm_names[t]);
            printf("%12s\t", "th");
            printf("%12.4f %12.4f %12.4f\t", dfb.th[t].min, dfb.th[t].max, dfb.th[t].avg);
            printf("%12.4f %12.4f %12.4f\t", 0., 0., 0.);

            int s_counts[N_DGACLASSES];
            source_count(stratosphere_sources, s_counts);
            
            printf("%12d %12d %12d\n", s_counts[0], s_counts[1], s_counts[2]);
            for (int cl = 0; cl < N_DGACLASSES; cl++) {
                if (cl == 1) continue;
                printf("\n%12d\t", cl);
                printf("%12d %12d %12d\t\t", (int32_t) dfb.falses[t][cl].min, (int32_t) dfb.falses[t][cl].max, (int32_t) dfb.falses[t][cl].avg);
                printf("%12.4f %12.4f %12.4f\t", dfb.false_ratio[t][cl].min, dfb.false_ratio[t][cl].max, dfb.false_ratio[t][cl].avg);
                printf("%12.4f %12.4f %12.4f\t", dfb.sources_trues[t][cl].min, dfb.sources_trues[t][cl].max, dfb.sources_trues[t][cl].avg);
            }
            #undef PRINTD
        }

        free(tests._);
    }


    free(psets._);
    free(stratosphere_sources._);

    for (int32_t i = 0; i < datasets.number; i++) {
        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
            free(datasets._[i].windows[cl]._);
        }
    }

    free(datasets._);
    */

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

    //             WindowRefs* ws = &dt_tt.train[CLASS__NOT_INFECTED];
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
    //                     for (int cl = 0; cl < N_DGACLASSES; ++cl) {
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
    //             for (int cl = 0; cl < N_DGACLASSES; ++cl) {
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