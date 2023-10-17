#include "dataset.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Ths {
    int32_t number;
    double* _;
} Ths;

DatasetTestThresoldChoiceMethod thcm[N_THCHOICEMETHODs] = {
    DTTCM_F1SCORE,
    DTTCM_F1SCORE_01,
    DTTCM_F1SCORE_05,
    DTTCM_FPR,
    DTTCM_TPR
};

char thcm_names[5][20] = {
    "F1SCORE_1.0",
    "F1SCORE_0.5",
    "F1SCORE_0.1",
    "FPR",
    "TPR"
};

void print_cm(const int N_PSETS, CM* cm) {
    for (int m = 0; m < N_PSETS; ++m) {
        printf(
            "\n| %5d, %5d |\n| %5d, %5d \n",
            cm->single[0].trues,
            cm->single[0].falses,
            cm->single[1].falses,
            cm->single[1].trues
        );
    }
}

int _ths_exist(int n, double ths[n], int th) {
    for (int32_t i = 0; i < n; i++) {
        if (ths[i] == th) return 1;
    }
    return 0;
}

Ths _get_ths(WindowRefs windows[N_CLASSES]) {
    int max = 0;
    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        max += windows[cl].number;
    }

    double* ths = calloc(max, sizeof(double));

    int n = 0;
    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        for (int32_t w = 0; w < windows[cl].number; w++) {
            int logit = floor(windows[cl]._[w]->logit);
            int exists = _ths_exist(n, ths, logit);
            if(!exists) {
                ths[n++] = logit;
            }
        }
    }

    Ths ths_ret;
    ths_ret.number = n;
    ths_ret._ = calloc(n, sizeof(double));
    memcpy(ths_ret._, ths, sizeof(double) * n);

    return ths_ret;
}

void _test(WindowRefs windows[N_CLASSES], double th, TF test[N_CLASSES]) {
    memset(test, 0, sizeof(TF) * N_CLASSES);
    for (int32_t cl = 0; cl < N_CLASSES; cl++) {

        for (int i = 0; i < windows[cl].number; i++) {
            int prediction = windows[cl]._[i]->logit >= th;
            int infected = cl > 0;

            if (prediction == infected) {
                test[cl].trues++;
            } else {
                test[cl].falses++;
            }
        }
    }
}

void _extract_wd(WSetRef* wrs_class, WSetRef* wrs_train, WSetRef* wrs_test) {
    int total = wrs_class->number;
    int extracted[total]; // 1 means extracted, 0 means not
    memset(extracted, 0, sizeof(int) * total);

    WSetRef* wrs_toextract;
    WSetRef* wrs_tofill;
    if (wrs_train->number > wrs_test->number) {
        wrs_toextract = wrs_train;
        wrs_tofill = wrs_test;
    } else {
        wrs_toextract = wrs_test;
        wrs_tofill = wrs_train;
    }

    {
        srand(time(NULL));
        int guard = total * 100;
        int guard_counter = 0;
        int i = 0;
        while (++guard_counter < guard && i < wrs_toextract->number) {
            int dado = rand() % total;

            if (extracted[dado]) continue;

            extracted[dado] = 1;
            wrs_toextract->_[i++] = wrs_class->_[dado];
            if (wrs_toextract->number < i) {
                printf("Error wrs_toextract->number <= i (%d <= %d)\n", wrs_toextract->number , i);
            }
        }

        if (i != wrs_toextract->number) {
            printf("Error %d != %d\n", i, wrs_toextract->number);
        }

        if (guard_counter >= guard) {
            printf("Guard error [%d >= %d] %d\n", guard_counter, guard, wrs_class->number);
        }
    }

    {
        int i = 0;
        for (int j = 0; j < total; ++j) {
            if (0 == extracted[j]) {
                wrs_tofill->_[i++] = wrs_class->_[j];
                if (wrs_tofill->number < i) {
                    printf("Error wrs_tofill->number <= i (%d <= %d)\n", wrs_tofill->number , i);
                }
            }
        }

        if (i != wrs_tofill->number) {
            printf("Error %d != %d\n", i, wrs_tofill->number);
        }
    }
}


void dataset_traintestsplit(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split, DatasetSplitMethod dsm) {
    assert((percentage_split > 0) & (percentage_split < 1));

    dt_tt->percentage_split = percentage_split;

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        WSetRef* wrs_train = &dt_tt->train[cl];
        WSetRef* wrs_test = &dt_tt->test[cl];

        if (dt->windows[cl].number == 0) {
            // printf("cl=%d\t%d\t%d\t%d\n", cl, dt->windows[cl].number, wrs_train->number, wrs_test->number);
            wrs_train->number = 0;
            wrs_train->_ = NULL;
            wrs_test->number = 0;
            wrs_test->_ = NULL;
            continue;
        }

        wrs_train->number = floor(dt->windows[cl].number * percentage_split);
        wrs_train->number = wrs_train->number == 0 ? 1 : wrs_train->number;
        wrs_train->_ = calloc(wrs_train->number, sizeof(Window*));

        wrs_test->number = dt->windows[cl].number - wrs_train->number;
        wrs_test->_ = calloc(wrs_test->number, sizeof(Window*));

        // printf("dt->windows[cl=%d].number\t%d\n", cl, dt->windows[cl].number);

        _extract_wd(&dt->windows[cl], wrs_train, wrs_test);
    }
}



// void dataset_traintestsplit_cm(int32_t wsize, PSets* psets, DatasetTrainTestPtr dt_tt) {

//     for (int m = 0; m < psets->number; ++m) {
//         double th = -1 * DBL_MAX;
//         WSetRef* ws_ni = &dt_tt->train[CLASS__NOT_INFECTED];

//         for (int i = 0; i < ws_ni->number; i++) {
//             double logit = ws_ni->_[i]->metrics._[m].logit;
//             if (th < logit) th = logit;
//         }

//         dt_tt->ths._[m] = th;


//     }   

//     for (int cl = 0; cl < N_CLASSES; ++cl) {
//         if (dt_tt->test->number == 0) continue;

//         WSetRef* wrs = &dt_tt->test[cl];

//         for (int i = 0; i < wrs->number; ++i) {
//             Window* window = wrs->_[i];

//             for (int m = 0; m < psets->number; ++m) {
//                 int32_t prediction; // 1 or 0
//                 int32_t binary_class;  // 1 or 0

//                 prediction = window->metrics._[m].logit >= dt_tt->ths._[m];
//                 binary_class = window->class == CLASS__NOT_INFECTED ? 0 : 1;

//                 // printf("%d,%d,%d,%s,%5.1f\t%5.1f\n", window->class, binary_class, prediction, prediction == binary_class ? "Goot" : "Wrong", window->metrics._[m].logit, dt_tt->ths._[m]);
//                 dt_tt->cms._[m].wsize = wsize;
//                 dt_tt->cms._[m].pset = &psets->_[m];
                
//                 if (prediction == binary_class) {
//                     dt_tt->cms._[m].single[binary_class].trues += 1;
//                     dt_tt->cms._[m].multi[window->class].trues += 1;
//                 } else {
//                     dt_tt->cms._[m].single[binary_class].falses += 1;
//                     dt_tt->cms._[m].multi[window->class].falses += 1;
//                 }
//             }
//         }
//     }
// }

WindowRefs _shuffle(Windows windows) {
    srand(time(NULL));

    int32_t number = windows.number;

    WindowRefs shuffled;
    shuffled._ = calloc(windows.number, sizeof(Window*));
    shuffled.number = 0;

    int guard = windows.number * 100;
    int guard_counter = 0;
    while (++guard_counter < guard && shuffled.number < windows.number) {
        int dado = rand() % number;

        if (shuffled._[dado]) continue;

        shuffled._[dado] = &windows._[dado];
        shuffled.number++;
    }

    return shuffled;
}

void dataset_folds(Dataset0* dt, DatasetFolds* df) {
    assert((df->balance_method != DSBM_EACH) || (df->split_method != DSM_IGNORE_1));

    const int32_t KFOLDs = df->kfolds;
    const int32_t TRAIN_KFOLDs = df->kfolds - df->test_folds;
    const int32_t TEST_KFOLDs = df->test_folds;

    WindowRefs windows[N_CLASSES];

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        if (df->shuffle) {
            windows[cl] = _shuffle(dt->windows[cl]);
        } else {
            windows[cl].number = dt->windows[cl].number;
            windows[cl]._ = calloc(dt->windows[cl].number, sizeof(Window*));
            for (int32_t w = 0; w < dt->windows[cl].number; w++) {
                windows[cl]._[w] = &dt->windows[cl]._[w];
            }
        }
    }


    int32_t kindexes[KFOLDs][KFOLDs];
    for (int32_t k = 0; k < KFOLDs; k++) {
        for (int32_t kindex = k; kindex < (k + TRAIN_KFOLDs); kindex++) {
            kindexes[k][kindex % KFOLDs] = 0;
        }
        for (int32_t kindex = k + TRAIN_KFOLDs; kindex < (k + KFOLDs); kindex++) {
            kindexes[k][kindex % KFOLDs] = 1;
        }
    }


    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        int32_t kfold_size = windows[cl].number / KFOLDs;
        int32_t kfold_size_rest = windows[cl].number - (kfold_size * KFOLDs);

        for (int32_t k = 0; k < KFOLDs; k++) {
            df->splits[k].kfold = k;

            int32_t train_size = kfold_size * TRAIN_KFOLDs;
            int32_t test_size = kfold_size * TEST_KFOLDs;
            if (kindexes[k][KFOLDs - 1]) {
                test_size += kfold_size_rest;
            } else {
                train_size += kfold_size_rest;
            }

            df->splits[k].train.multi[cl].number = train_size;
            df->splits[k].train.multi[cl]._ = calloc(train_size, sizeof(Window*));

            df->splits[k].test.multi[cl].number = test_size;
            df->splits[k].test.multi[cl]._ = calloc(test_size, sizeof(Window*));

            int32_t train_index = 0;
            int32_t test_index = 0;
            for (int32_t kk = 0; kk < KFOLDs; kk++) {

                int32_t start = kk * kfold_size;
                int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&df->splits[k].train.multi[cl]._[train_index], &windows[cl]._[start], sizeof(Window*) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&df->splits[k].test.multi[cl]._[test_index], &windows[cl]._[start], sizeof(Window*) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }


    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        free(windows[cl]._);
    }

    // for (int32_t k = 0; k < KFOLDs; k++) {
    //     for (int32_t cl = 0; cl < N_CLASSES; cl++) {
    //         for (int32_t w = 0; w < df->splits[k].train.multi[cl].number; w++) {
    //             printf("%d, %d, %d, %g\n", k, cl, w, df->splits[k].train.multi[cl]._[w]->logit);
    //         }
    //         for (int32_t w = 0; w < df->splits[k].test.multi[cl].number; w++) {
    //             printf("%d, %d, %d, %g\n", k, cl, w, df->splits[k].test.multi[cl]._[w]->logit);
    //         }
    //     }
    // }
}

void dataset_folds_free(DatasetFolds* df) {
    for (int32_t k = 0; k < df->kfolds; k++) {
        for (int32_t cl = 0; cl < N_CLASSES; cl++) {
            free(df->splits[k].test.multi[cl]._);
            free(df->splits[k].train.multi[cl]._);
        }
    }
    free(df->splits);
}

double _f1score_beta(TF tf[N_CLASSES], double beta) {
    double tn = tf[0].trues;
    double fp = tf[0].falses;
    double fn = tf[2].falses;
    double tp = tf[2].trues;

    double beta_2 = beta * beta;

    return ((double) ((1 + beta_2) * tp)) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
}

void dataset_splittest(DatasetSplit* ds, DatasetTest* test) {
    memset(test, 0, sizeof(DatasetTest));

    double* th = test->ths;

    {
        WindowRefs windows_0 = ds->train.multi[0];
        th[DTTCM_FPR] = -1 * DBL_MAX;
        th[DTTCM_TPR] = 1 * DBL_MAX;
        for (int i = 0; i < windows_0.number; i++) {
            Window* window = windows_0._[i];
            double logit = window->logit;
            if (th[DTTCM_FPR] < logit) th[DTTCM_FPR] = ceil(logit);
            if (th[DTTCM_TPR] >= logit) th[DTTCM_TPR] = floor(logit);
        }
    }

    double f1_score_1 = 0;
    double f1_score_01 = 0;
    double f1_score_05 = 0;
    Ths ths = _get_ths(ds->train.multi);
    for (int t = 0; t < ths.number; t++) {
        TF multi[N_CLASSES];
        _test(ds->train.multi, ths._[t], multi);
        {
            double f1_score = _f1score_beta(multi, 1);
            if (f1_score_1 < f1_score) {
                f1_score_1 = f1_score;
                th[DTTCM_F1SCORE] = ths._[t];
            }
        }
        {
            double f1_score = _f1score_beta(multi, 0.5);
            if (f1_score_05 < f1_score) {
                f1_score_05 = f1_score;
                th[DTTCM_F1SCORE_05] = ths._[t];
            }
        }
        {
            double f1_score = _f1score_beta(multi, 0.1);
            if (f1_score_01 < f1_score) {
                f1_score_01 = f1_score;
                th[DTTCM_F1SCORE_01] = ths._[t];
            }
        }
    }
    free(ths._);

    for (int t = 0; t < N_THCHOICEMETHODs; t++) {
        _test(ds->test.multi, th[t], test->multi[t]);
    }
}

// void dataset_test_free(DatasetTests tests) {
//     for (int32_t k = 0; k < tests.number; k++) {
//         for (int32_t k = 0; k < tests._[k].ths; k++) {
//         }
//     }
// }

DatasetTests dataset_foldstest(Dataset0* dt, DatasetFoldsConfig df_config) {
    DatasetFolds df;

    df.balance_method = df_config.balance_method;
    df.kfolds = df_config.kfolds;
    df.shuffle = df_config.shuffle;
    df.split_method = df_config.split_method;
    df.test_folds = df_config.test_folds;

    df.splits = calloc(df.kfolds, sizeof(DatasetSplit));

    dataset_folds(dt, &df);

    DatasetTests tests;
    tests.number = df.kfolds;
    tests._ = calloc(df.kfolds, sizeof(DatasetTest));

    for (int32_t k = 0; k < df.kfolds; k++) {
        dataset_splittest(&df.splits[k], &tests._[k]);
    }

    dataset_folds_free(&df);

    return tests;
}