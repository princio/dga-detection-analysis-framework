#include "experiment.h"

#include "dataset.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AVG_INIT(N, MASK)  \
        cmavgs[MASK].ratios._ = (Ratio*) calloc(N, sizeof(Ratio));\
        cmavgs[MASK].mask = (MASK);\
        cmavgs[MASK].ratios.number = (N);

// 0: ( 1, 0, 0, 0 )   |   SPLIT                        |
// 1: ( 1, 0, 0, 1 )   |   SPLIT, METRICs               |
// 2: ( 1, 0, 1, 0 )   |   SPLIT, KFOLD                 |
// 3: ( 1, 0, 1, 1 )   |   SPLIT, KFOLD, METRICs        |
// 4: ( 1, 1, 0, 0 )   |   SPLIT, WSIZE                 |
// 5: ( 1, 1, 0, 1 )   |   SPLIT, WSIZE, METRIC         |
// 6: ( 1, 1, 1, 0 )   |   SPLIT, WSIZE, KFOLD          |
// 7: ( 1, 1, 1, 1 )   |   SPLIT, WSIZE, KFOLD, METRIC  |


const int AVGG_all = 0;
const int AVGGroups_MASK_W = 1;
const int AVGGroups_MASK_K = 2;
const int AVGGroups_MASK_M = 4;


const char AVGGroups_names[4][10] = {
    "split",
    "wsize",
    "KFOLD",
    "metrics",
};


const int N_AVGGroups = 8;

const int ncursor = 4;



double evfn_f1score_beta(Prediction pr, double beta) {
    double tn = pr.single[0].trues;
    double fp = pr.single[0].falses;
    double fn = pr.single[1].falses;
    double tp = pr.single[1].trues;

    double beta_2 = beta * beta;

    return ((double) ((1 + beta_2) * tp)) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
}

double evfn_f1score_1(Prediction pr) {
    return evfn_f1score_beta(pr, 1);
}

double evfn_f1score_beta_05(Prediction pr) {
    return evfn_f1score_beta(pr, 0.5);
}

double evfn_f1score_beta_01(Prediction pr) {
    return evfn_f1score_beta(pr, 0.1);
}

double evfn_fpr(Prediction pr) {
    double tn = pr.single[0].trues;
    double fp = pr.single[0].falses;

    return (fp) / (tn + fp);
}

double evfn_tpr(Prediction pr) {
    double fn = pr.single[1].falses;
    double tp = pr.single[1].trues;

    return (tp) / (tp + fn);
}

void cmavg_actives(const int avgmask, int actives[4], int* last_active) {
    for (int i = 0; i < ncursor; i++) actives[i] = 0;

    actives[0] = 1;
    for (int i = 1; i < ncursor; i++) {
        int active = avgmask & (1 << (i-1)) ? 1 : 0;
        actives[i] = active;
        if (active)
            *last_active = i;
    }
}


void cmavg_cursor_name(char name[100], int avgmask) {

    if (avgmask == AVGG_all) {
        sprintf(name, "all");
        return;
    }

    int actives[ncursor];
    int last = 0;
    char space[2] = ",";

    cmavg_actives(avgmask, actives, &last);
    
    // sprintf(name, "%d\t[%d,%d,%d]\t%d\t", avgmask, actives[1], actives[2], actives[3], avgmask);

    for (int i = 1; i < ncursor; i++) {
        if (actives[i]) {
            strcat(name, AVGGroups_names[i]);
            if (i != last) strcat(name, space);
        }
    }
}


int cmavg_cursor(const int cursor[4], const int sizes[4], int avg) {
    int actives[ncursor];
    int last = 0;

    cmavg_actives(avg, actives, &last);

    int c = 0;
    for (int l = 0; l < last; l++) {
        int row = 1;
        if (cursor[l] == 0) continue;
        if (actives[l] == 0) continue;
        for (int ll = l + 1; ll <= last; ll++) {
            if (actives[ll]) row *= sizes[ll];
        }
        c += row * cursor[l];
    }
    
    c += cursor[last];

    return c;
}

AVG* cmavg_init(ExperimentSet* es) {
    const int N_SPLITRATIOs = es->N_SPLITRATIOs;
    const int KFOLDs = es->KFOLDs;
    const int N_METRICs = es->windowing->psets.number;
    const int N_WSIZEs = es->windowing->wsizes.number;

    AVG* cmavgs = calloc(8, sizeof(AVG));

    AVG_INIT(N_SPLITRATIOs, AVGG_all);
    AVG_INIT(N_SPLITRATIOs * N_METRICs, AVGGroups_MASK_M);
    AVG_INIT(N_SPLITRATIOs * KFOLDs, AVGGroups_MASK_K);
    AVG_INIT(N_SPLITRATIOs * KFOLDs * N_METRICs, AVGGroups_MASK_K + AVGGroups_MASK_M);
    AVG_INIT(N_SPLITRATIOs * N_WSIZEs, AVGGroups_MASK_W);
    AVG_INIT(N_SPLITRATIOs * N_WSIZEs * N_METRICs, AVGGroups_MASK_W + AVGGroups_MASK_M);
    AVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs, AVGGroups_MASK_W + AVGGroups_MASK_K);
    AVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs, AVGGroups_MASK_W + AVGGroups_MASK_K + AVGGroups_MASK_M);

    return cmavgs;
}

WindowingPtr experiment_run(Experiment experiment) {

    WindowingPtr windowing_ptr = NULL;

    windowing_ptr = windowing_load(rootpath, name);

    if (windowing_ptr) return windowing_ptr;

    windowing_ptr = windowing_run(experiment, wsizes, psetgenerator);

    windowing_save(windowing_ptr);

    return windowing_ptr;
}


void experiment_load_sources(Experiment experiment) {

}


void _fprintf_cm(FILE* fp, Ratio* ratio) {
    // printf(
    //     "\n| %5d, %5d |\n| %5d, %5d |\n",
    //     cm->classes[0][1],
    //     cm->classes[0][0],
    //     cm->classes[2][0],
    //     cm->classes[2][1]
    // );
    fprintf(
        fp,
        "%d,%4.3f,%4.3f",
        ratio->nwindows,
        // ratio->single_ratio.zeros,
        // ratio->single_ratio.falses_nan,
        // ratio->single_ratio.trues_nan,
        ratio->single_ratio.relative[1] / ratio->nwindows
    );
}


void experiment_test(ExperimentSet* es) {
    assert(es->N_SPLITRATIOs <= MAX_SPLITPERCENTAGES);

    WindowingPtr windowing = es->windowing;

    const int N_SPLITRATIOs = es->N_SPLITRATIOs;
    const int KFOLDs = es->KFOLDs;
    const double* split_percentages = es->split_percentages;
    const int N_WSIZEs = windowing->wsizes.number;
    const int N_METRICs = windowing->psets.number;

    Dataset *dt = calloc(windowing->wsizes.number, sizeof(Dataset));
    dataset_fill(windowing, dt);


    Prediction cm_eval[N_SPLITRATIOs][N_WSIZEs][KFOLDs][N_METRICs];
    memset(cm_eval, 0, sizeof(Prediction) * N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs);

    Prediction cm_test[N_SPLITRATIOs][N_WSIZEs][KFOLDs][N_METRICs];
    memset(cm_test, 0, sizeof(Prediction) * N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs);

    int32_t S = N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs;
    int32_t tester[S];
    memset(tester, 0, S);


    char path[700];
    sprintf(path, "%s/%s/dt_tt.csv", windowing->rootpath, windowing->name);
    FILE* fpdttt = fopen(path, "w");
    for (int p = 0; p < N_SPLITRATIOs; ++p) {
        double split_percentage = split_percentages[p];

        for (int w = 0; w < N_WSIZEs; ++w) {
            const int32_t wsize = windowing->wsizes._[w];

            for (int k = 0; k < KFOLDs; ++k) {

                DatasetTrainTest dt_tt;
                memset(&dt_tt, 0, sizeof(DatasetTrainTest));

                dataset_traintestsplit(&dt[w], &dt_tt, split_percentage);

                double ths[N_METRICs];
                for (int __i = 0; __i < N_METRICs; ++__i) ths[__i] = -1 * DBL_MAX;

                dt_tt.eval.number = N_METRICs;
                dt_tt.eval._ = cm_eval[p][w][k];

                dt_tt.test.number = N_METRICs;
                dt_tt.test._ = cm_test[p][w][k];

                WSetRef* ws_notinfected = &dt_tt.train[CLASS__NOT_INFECTED];

                assert(ws_notinfected->number > 0);

                for (int i = 0; i < ws_notinfected->number; i++) {
                    Window* window = ws_notinfected->_[i];
                    for (int m = 0; m < window->metrics.number; m++) {
                        double logit = window->metrics._[m].logit;
                        if (ths[m] < logit) ths[m] = logit;
                    }
                }

                dataset_traintestsplit_cm(wsize, &windowing->psets, &dt_tt);

                {
                    fprintf(fpdttt, "%g,%d,", split_percentage, wsize);
                    int32_t y;
                    for (y = 0; y < N_METRICs - 1; y++)
                    {
                        fprintf(fpdttt, "%d,%d/%d,", dt_tt.train->number, dt_tt.cms._[y].single->falses, dt_tt.cms._[y].single->trues);
                    }
                    fprintf(fpdttt, "%d/%d\n", dt_tt.cms._[y].single->falses, dt_tt.cms._[y].single->trues);
                }


                {
                    fprintf(fpdttt, ",,", split_percentage, wsize);
                    int32_t y;
                    for (y = 0; y < N_METRICs - 1; y++)
                    {
                        fprintf(fpdttt, "%d,%g,", dt_tt.train->number, ths[y]);
                    }
                    fprintf(fpdttt, "%g\n", ths[y]);
                }
            }
        }
    }
    fclose(fpdttt);

    {
        AVG *cmavgs = cmavg_init(es);

        const int sizes[4] = {
            N_SPLITRATIOs,
            N_WSIZEs,
            KFOLDs,
            N_METRICs
        };

        int cursors[4];
        for (cursors[0] = 0; cursors[0] < N_SPLITRATIOs; ++cursors[0]) {

            for (cursors[1] = 0; cursors[1] < N_WSIZEs; ++cursors[1]) {

                for (cursors[2] = 0; cursors[2] < KFOLDs; ++cursors[2]) {

                    for (cursors[3] = 0; cursors[3] < N_METRICs; ++cursors[3]) {
                        ConfusionMatrix* _cm = &cm[cursors[0]][cursors[1]][cursors[2]][cursors[3]];

                        for (int cl = 0; cl < N_CLASSES; ++cl) {

                            int32_t single_falses = _cm->single[cl != CLASS__NOT_INFECTED].falses;
                            int32_t single_trues = _cm->single[cl != CLASS__NOT_INFECTED].trues;

                            double single_false_ratio = ((double) single_falses) / (single_falses + single_trues);
                            double single_true_ratio = ((double) single_trues) / (single_falses + single_trues);

                            int32_t multi_falses = _cm->multi[cl].falses;
                            int32_t multi_trues = _cm->multi[cl].trues;

                            double multi_false_ratio = ((double) multi_falses) / (multi_falses + multi_trues);
                            double multi_true_ratio = ((double) multi_trues) / (multi_falses + multi_trues);

                            printf("%d\t%d\t%d\n", cl, single_falses, single_trues);
                            printf("%d\t%d\t%d\n", cl, multi_falses, multi_trues);

                            assert((single_falses + single_trues) > 0);
                            assert((multi_falses + multi_trues) > 0);

                            for (int avgmask = 0; avgmask < N_AVGGroups; ++avgmask) {
                                int cursor = cmavg_cursor(cursors, sizes, avgmask);

                                Ratio* ratio = &cmavgs[avgmask].ratios._[cursor];

                                ratio->nwindows++;
                                
                                ratio->single_ratio.absolute[0] += single_falses;
                                ratio->single_ratio.absolute[1] += single_trues;
                                ratio->single_ratio.relative[0] += single_false_ratio;
                                ratio->single_ratio.relative[1] += single_true_ratio;

                                ratio->multi_ratio[cl].absolute[0] += multi_falses;
                                ratio->multi_ratio[cl].absolute[1] += multi_trues;
                                ratio->multi_ratio[cl].relative[0] += multi_false_ratio;
                                ratio->multi_ratio[cl].relative[1] += multi_true_ratio;
                            }
                        }
                    }
                }
            }
        }


        char path[700];
        sprintf(path, "%s/%s/avg.csv", windowing->rootpath, windowing->name);
        FILE* fp = fopen(path, "w");

        for (cursors[0] = 0; cursors[0] < N_SPLITRATIOs; ++cursors[0]) {

            for (cursors[1] = 0; cursors[1] < N_WSIZEs; ++cursors[1]) {

                for (cursors[2] = 0; cursors[2] < KFOLDs; ++cursors[2]) {

                    for (cursors[3] = 0; cursors[3] < N_METRICs; ++cursors[3]) {

                        for (int avgmask = 0; avgmask < N_AVGGroups; ++avgmask) {
                            int cursor = cmavg_cursor(cursors, sizes, avgmask);
                            char name[100] = "";

                            int actives[4];
                            int last_active;

                            cmavg_cursor_name(name, avgmask);

                            cmavg_actives(avgmask, actives, &last_active);

                            // printf("%s:", name);

                            for (int i = 0; i < ncursor; ++i) {
                                if (actives[i]) {
                                    switch (i) {
                                        case 0:
                                            fprintf(fp, "%g", split_percentages[cursors[0]]);
                                            break;
                                        case 1:
                                            fprintf(fp, "%d", windowing->wsizes._[cursors[1]]);
                                            break;
                                        case 2:
                                            fprintf(fp, "%d", cursors[2]);
                                            break;
                                        case 3:
                                            fprintf(fp, "%d", windowing->psets._[cursors[3]].id);
                                            break;
                                    }
                                } else {
                                    fprintf(fp, "*");
                                }
                                fprintf(fp, ",");
                            }

                            _fprintf_cm(fp, &cmavgs[avgmask].ratios._[cursor]);

                            fprintf(fp, "\n");

                        }
                    }
                }
            }
        }
        
        // exit(0);


        // for (cursors[0] = 0; cursors[0] < N_SPLITRATIOs; ++cursors[0]) {
        //     for (cursors[1] = 0; cursors[1] < N_WSIZEs; ++cursors[1]) {
        //         for (cursors[2] = 0; cursors[2] < KFOLDs; ++cursors[2]) {
        //             for (cursors[3] = 0; cursors[3] < N_METRICs; ++cursors[3]) {
        //                 for (int avgs_mask = 0; avgs_mask < N_AVGGroups; ++avgs_mask) {
        //                     int cursor = cmavg_cursor(cursors, sizes, avgs_mask);
        //                     for (int cl = 0; cl < N_CLASSES; ++cl) {
        //                         int f = cmavgs[avgs_mask].cms._[cursor].classes[cl][0];
        //                         int t = cmavgs[avgs_mask].cms._[cursor].classes[cl][1];
        //                         cmavgs[avgs_mask].cms._[cursor].false_ratio[cl] = f / (t + f);
        //                         cmavgs[avgs_mask].cms._[cursor].true_ratio[cl] = t / (t + f);

        //                         printf("%5d, %5d, %5d, %5d, %5d, %7.5f, %7.5f\n",
        //                             cursors[0],
        //                             cursors[1],
        //                             cursors[2],
        //                             cursors[3],
        //                             avgs_mask,
        //                             cmavgs[avgs_mask].cms._[cursor].false_ratio[cl], cmavgs[avgs_mask].cms._[cursor].true_ratio[cl]);
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }
    }

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
}
