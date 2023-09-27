#include "experiment.h"

#include "dataset.h"
#include "windowing.h"

#include <stdlib.h>
#include <string.h>


#define CMAVG_INIT(N, ID)  \
        cmavgs[ID]._ = (CM*) calloc(N, sizeof(CM));\
        cmavgs[ID].separate_id = (ID);\
        cmavgs[ID].totals = (N);

        // 0: ( 1, 0, 0, 0 )   |   SPLIT                        | WSIZE * METRICs * KFOLD
        // 1: ( 1, 0, 0, 1 )   |   SPLIT, METRICs               | WSIZE * METRICs * KFOLD
        // 2: ( 1, 0, 1, 0 )   |   SPLIT, KFOLD                 |
        // 3: ( 1, 0, 1, 1 )   |   SPLIT, KFOLD, METRICs        |
        // 4: ( 1, 1, 0, 0 )   |   SPLIT, WSIZE                 |
        // 5: ( 1, 1, 0, 1 )   |   SPLIT, WSIZE, METRIC         |
        // 6: ( 1, 1, 1, 0 )   |   SPLIT, WSIZE, KFOLD          | 
        // 7: ( 1, 1, 1, 1 )   |   SPLIT, WSIZE, KFOLD, METRIC  |

const int AVGGroups_MASK_W = 1;
const int AVGGroups_MASK_K = 2;
const int AVGGroups_MASK_M = 4;

const int AVGG_all = 0;
const int AVGG_M =  AVGGroups_MASK_M;
const int AVGG_K =  AVGGroups_MASK_K;
const int AVGG_KM =  AVGGroups_MASK_K | AVGGroups_MASK_M;
const int AVGG_W =  AVGGroups_MASK_W;
const int AVGG_WM =  AVGGroups_MASK_W | AVGGroups_MASK_M;
const int AVGG_WK =  AVGGroups_MASK_W | AVGGroups_MASK_K;
const int AVGG_WKM =  AVGGroups_MASK_W | AVGGroups_MASK_K | AVGGroups_MASK_M;

int cmavg_cursor(ExperimentSet es, int cursor[4], int avg) {
    int c = 0;

    const int sizes[3] = { es.windowing->wsizes.number, es.KFOLDs, es.windowing->psets.number };

    const int ncursor = 4;
    int active[ncursor];
    int last;

    for (int i = 0; i < ncursor; i++) active[i] = 0;

    for (int i = 0; i < ncursor; --i) {
        active[i] = avg & (1 << i);
        last = active[i] ? i : last;
    }

    for (int l = 0; l < last; l++) {
        int row = 1;
        for (int ll = l; ll < last; ll++) {
            row *= active[ll] * sizes[ll];
        }
        c += row * cursor[l];
    }
    
    c += cursor[last];

}

CMAVG* cmavg_init(ExperimentSet es) {
    const int N_SPLITRATIOs = es.N_SPLITRATIOs;
    const int KFOLDs = es.KFOLDs;
    const int N_METRICs = es.windowing->psets.number;
    const int N_WSIZEs = es.windowing->wsizes.number;

    CMAVG* cmavgs = calloc(8, sizeof(CMAVG));

    CMAVG_INIT(N_SPLITRATIOs, AVGG_all);
    CMAVG_INIT(N_SPLITRATIOs * N_METRICs, AVGG_M);
    CMAVG_INIT(N_SPLITRATIOs * KFOLDs, AVGG_K);
    CMAVG_INIT(N_SPLITRATIOs * KFOLDs * N_METRICs, AVGG_KM);
    CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs, AVGG_W);
    CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * N_METRICs, AVGG_WM);
    CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs, AVGG_WK);
    CMAVG_INIT(N_SPLITRATIOs * N_WSIZEs * KFOLDs * N_METRICs, AVGG_WKM);
}

// void cmavg_fill(enum AVGGroup avg, ExperimentSet es, ConfusionMatrix* cms, int s, int k, int m, int falses, int trues) {

//     CMAvgCursor cursor;
//     for (cursor.split = 0; cursor.split < es.N_SPLITRATIOs; ++cursor.split) {
//         for (cursor.wsize = 0; cursor.wsize < es.windowing->wsizes.number; ++cursor.wsize) {
//             for (cursor.kfold = 0; cursor.kfold < es.KFOLDs; ++cursor.kfold) {
//                 for (cursor.metric = 0; cursor.metric < es.windowing->psets.number; ++cursor.metric) {
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
// }

WindowingPtr experiment_run(char* rootpath, char*name, WSizes wsizes, PSetGenerator* psetgenerator) {

    WindowingPtr windowing_ptr = NULL;

    windowing_ptr = windowing_load(rootpath, name);

    if (windowing_ptr) return windowing_ptr;

    windowing_ptr = windowing_run(rootpath, name, wsizes, psetgenerator);

    windowing_save(windowing_ptr);

    return windowing_ptr;
}

void experiment_tests(WindowingPtr windowing, ExperimentSet es) {
    const int N_SPLITRATIOs = es.N_SPLITRATIOs;
    const int KFOLDs = es.KFOLDs;
    const double* split_percentages = es.split_percentages;

    Dataset *dt = calloc(windowing->wsizes.number, sizeof(Dataset));
    dataset_fill(windowing, dt);

    ConfusionMatrix cm[N_SPLITRATIOs][windowing->wsizes.number][KFOLDs][windowing->psets.number];

    for (int p = 0; p < N_SPLITRATIOs; ++p) {
        double split_percentage = split_percentages[p];
        for (int w = 0; w < windowing->wsizes.number; ++w) {
            const int32_t wsize = windowing->wsizes._[w];
            for (int k = 0; k < KFOLDs; ++k) {
                DatasetTrainTest dt_tt;
                dataset_traintest(&dt[w], &dt_tt, split_percentage);

                double ths[windowing->psets.number];
                memset(ths, 0, sizeof(double) * windowing->psets.number);

                WSetRef* ws = &dt_tt.train[CLASS__NOT_INFECTED];
                for (int i = 0; i < ws->number; i++)
                {
                    Window* window = ws->_[i];
                    for (int m = 0; m < window->metrics.number; m++) {
                        double logit = window->metrics._[m].logit;
                        if (ths[m] < logit) ths[m] = logit;
                    }
                }

                dataset_traintest_cm(wsize, &windowing->psets, &dt_tt, windowing->psets.number, ths, &cm[p][w][k][0]); 
            }
        }
    }


    {
        const int32_t N_WSIZEs = windowing->wsizes.number;
        const int32_t N_METRICs = windowing->psets.number;

        CMAVG *cmavgs = cmavg_init(es);


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
        CMAvgCursor cursor;
        for (cursor.split = 0; cursor.split < N_SPLITRATIOs; ++cursor.split) {
            for (cursor.wsize = 0; cursor.wsize < N_WSIZEs; ++cursor.wsize) {
                for (cursor.kfold = 0; cursor.kfold < KFOLDs; ++cursor.kfold) {
                    for (cursor.metric = 0; cursor.metric < N_METRICs; ++cursor.metric) {
                        for (int cl = 0; cl < N_CLASSES; ++cl) {
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
}