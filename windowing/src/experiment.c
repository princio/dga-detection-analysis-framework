#include "experiment.h"

#include "dataset.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AVG_INIT(N, MASK)  \
        cmavgs[MASK].cms._ = (CM*) calloc(N, sizeof(CM));\
        cmavgs[MASK].mask = (MASK);\
        cmavgs[MASK].cms.number = (N);

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

WindowingPtr experiment_run(char* rootpath, char*name, WSizes wsizes, PSetGenerator* psetgenerator) {

    WindowingPtr windowing_ptr = NULL;

    windowing_ptr = windowing_load(rootpath, name);

    if (windowing_ptr) return windowing_ptr;

    windowing_ptr = windowing_run(rootpath, name, wsizes, psetgenerator);

    windowing_save(windowing_ptr);

    return windowing_ptr;
}


void _print_cm(CM* cm) {
    // printf(
    //     "\n| %5d, %5d |\n| %5d, %5d |\n",
    //     cm->classes[0][1],
    //     cm->classes[0][0],
    //     cm->classes[2][0],
    //     cm->classes[2][1]
    // );
    printf(
        "%d,%d,%d,%d,%4.3f,%4.3f",
        cm->classes[0][1],
        cm->classes[0][0],
        cm->classes[2][0],
        cm->classes[2][1],
        ((double)cm->classes[0][1]) / (cm->classes[0][1] + cm->classes[0][0]),
        ((double)cm->classes[2][1]) / (cm->classes[2][1] + cm->classes[2][0])
    );
}


void experiment_test(ExperimentSet* es) {
    assert(es->N_SPLITRATIOs <= MAX_SPLITPERCENTAGES);

    WindowingPtr windowing = es->windowing;

    const int N_SPLITRATIOs = es->N_SPLITRATIOs;
    const int KFOLDs = es->KFOLDs;
    const double* split_percentages = es->split_percentages;

    Dataset *dt = calloc(windowing->wsizes.number, sizeof(Dataset));
    dataset_fill(windowing, dt);

    ConfusionMatrix cm[N_SPLITRATIOs][windowing->wsizes.number][KFOLDs][windowing->psets.number];
    memset(cm, 0, sizeof(ConfusionMatrix) * N_SPLITRATIOs * windowing->wsizes.number * KFOLDs * windowing->psets.number);

    for (int p = 0; p < N_SPLITRATIOs; ++p) {
        double split_percentage = split_percentages[p];

        for (int w = 0; w < windowing->wsizes.number; ++w) {
            const int32_t wsize = windowing->wsizes._[w];

            for (int k = 0; k < KFOLDs; ++k) {

                DatasetTrainTest dt_tt;
                memset(&dt_tt, 0, sizeof(DatasetTrainTest));

                dataset_traintestsplit(&dt[w], &dt_tt, split_percentage);

                double ths[windowing->psets.number];
                for (int __i = 0; __i < windowing->psets.number; ++__i) ths[__i] = -1 * DBL_MAX;

                dt_tt.ths.number = windowing->psets.number;
                dt_tt.cms.number = windowing->psets.number;

                dt_tt.ths._ = ths;
                dt_tt.cms._ = &cm[p][w][k][0];

                WSetRef* ws = &dt_tt.train[CLASS__NOT_INFECTED];

                if (ws->number == 0) {
                    continue;
                }

                for (int i = 0; i < ws->number; i++) {
                    Window* window = ws->_[i];
                    for (int m = 0; m < window->metrics.number; m++) {
                        double logit = window->metrics._[m].logit;
                        if (ths[m] < logit) ths[m] = logit;
                    }
                }

                dataset_traintestsplit_cm(wsize, &windowing->psets, &dt_tt);

            }
        }
    }

    {
        const int32_t N_WSIZEs = windowing->wsizes.number;
        const int32_t N_METRICs = windowing->psets.number;

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

                        for (int avgmask = 0; avgmask < N_AVGGroups; ++avgmask) {
                            int cursor = cmavg_cursor(cursors, sizes, avgmask);

                            for (int cl = 0; cl < N_CLASSES; ++cl) {
                                cmavgs[avgmask].cms._[cursor].classes[cl][0] += cm[cursors[0]][cursors[1]][cursors[2]][cursors[3]].classes[cl][0];
                                cmavgs[avgmask].cms._[cursor].classes[cl][1] += cm[cursors[0]][cursors[1]][cursors[2]][cursors[3]].classes[cl][1];
                            }
                        }
                    }
                }
            }
        }


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
                                            printf("%g", split_percentages[cursors[0]]);
                                            break;
                                        case 1:
                                            printf("%3d", windowing->wsizes._[cursors[1]]);
                                            break;
                                        case 2:
                                            printf("%3d", cursors[2]);
                                            break;
                                        case 3:
                                            printf("%3d", windowing->psets._[cursors[3]].id);
                                            break;
                                    }
                                } else {
                                    printf("*");
                                }
                                printf(",");
                            }

                            _print_cm(&cmavgs[avgmask].cms._[cursor]);

                            printf("\n");

                        }
                    }
                }
            }
        }
        
        exit(0);


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
