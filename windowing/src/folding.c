#include "folding.h"

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

FoldingTestThresoldChoiceMethod thcm[N_THCHOICEMETHODs] = {
    FOLDING_DTTCM_F1SCORE,
    FOLDING_DTTCM_F1SCORE_01,
    FOLDING_DTTCM_F1SCORE_05,
    FOLDING_DTTCM_FPR,
    FOLDING_DTTCM_TPR
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

double _f1score_beta(Test tf[N_CLASSES], double beta) {
    // double tn = tf[0].trues;
    double fp = tf[0].windows.falses;
    double fn = tf[2].windows.falses;
    double tp = tf[2].windows.trues;

    double beta_2 = beta * beta;

    return ((double) ((1 + beta_2) * tp)) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
}

void _test(Dataset* ds, WindowRefs windows[N_CLASSES], double th, Test test[N_CLASSES]) {
    memset(test, 0, sizeof(TF) * N_CLASSES);

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        test[cl].sources = calloc(ds->sources.arrays->multi[cl].number, sizeof(int32_t));
        
        for (int32_t i = 0; i < windows[cl].number; i++) {
            Window* window = windows[cl]._[i];
            int prediction = window->logit >= th;
            int infected = cl > 0;

            if (prediction == infected) {
                test[cl].windows.trues++;
                test[cl].sources[window->source_classindex].trues++;
            } else {
                test[cl].windows.falses++;
                test[cl].sources[window->source_classindex].falses++;
            }
        }
    }
}

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


void folding_datasets(Dataset* ds, Folding* folding) {
    assert((folding->config.balance_method != FOLDING_DSBM_EACH) || (folding->config.split_method != FOLDING_DSM_IGNORE_1));

    WindowRefs windows[N_CLASSES];
    const int32_t KFOLDs = folding->config.kfolds;
    const int32_t TRAIN_KFOLDs = folding->config.kfolds - folding->config.test_folds;
    const int32_t TEST_KFOLDs = folding->config.test_folds;

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        if (folding->config.shuffle) {
            windows[cl] = _shuffle(ds->windows[cl]);
        } else {
            windows[cl].number = ds->windows[cl].number;
            windows[cl]._ = calloc(ds->windows[cl].number, sizeof(Window*));
            for (int32_t w = 0; w < ds->windows[cl].number; w++) {
                windows[cl]._[w] = &ds->windows[cl]._[w];
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

            int32_t train_size;
            int32_t test_size;
            FoldingDataset* fd;

            train_size = kfold_size * TRAIN_KFOLDs;
            test_size = kfold_size * TEST_KFOLDs;
            fd = &folding->items[k].dataset;

            fd->kfold = k;

            if (kindexes[k][KFOLDs - 1]) {
                test_size += kfold_size_rest;
            } else {
                train_size += kfold_size_rest;
            }

            fd->train.multi[cl].number = train_size;
            fd->train.multi[cl]._ = calloc(train_size, sizeof(Window*));

            fd->test.multi[cl].number = test_size;
            fd->test.multi[cl]._ = calloc(test_size, sizeof(Window*));

            int32_t train_index = 0;
            int32_t test_index = 0;
            for (int32_t kk = 0; kk < KFOLDs; kk++) {

                int32_t start = kk * kfold_size;
                int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&fd->train.multi[cl]._[train_index], &windows[cl]._[start], sizeof(Window*) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&fd->test.multi[cl]._[test_index], &windows[cl]._[start], sizeof(Window*) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        free(windows[cl]._);
    }
}

void folding_test(Dataset* ds, FoldingItem* item) {
    FoldingDataset* fds = &item->dataset;
    FoldingTest* test = &item->test;
    
    memset(&test, 0, sizeof(FoldingTest));

    double* th = test->ths;

    {
        WindowRefs windows_0 = fds->train.multi[0];
        th[FOLDING_DTTCM_FPR] = -1 * DBL_MAX;
        th[FOLDING_DTTCM_TPR] = 1 * DBL_MAX;
        for (int i = 0; i < windows_0.number; i++) {
            Window* window = windows_0._[i];
            double logit = window->logit;
            if (th[FOLDING_DTTCM_FPR] < logit) th[FOLDING_DTTCM_FPR] = ceil(logit);
            if (th[FOLDING_DTTCM_TPR] >= logit) th[FOLDING_DTTCM_TPR] = floor(logit);
        }
    }

    double f1_score_1 = 0;
    double f1_score_01 = 0;
    double f1_score_05 = 0;
    Ths ths = _get_ths(fds->train.multi);
    for (int t = 0; t < ths.number; t++) {
        Test multi[N_CLASSES];
        _test(ds, fds->train.multi, ths._[t], multi);
        {
            double f1_score = _f1score_beta(multi, 1);
            if (f1_score_1 < f1_score) {
                f1_score_1 = f1_score;
                th[FOLDING_DTTCM_F1SCORE] = ths._[t];
            }
        }
        {
            double f1_score = _f1score_beta(multi, 0.5);
            if (f1_score_05 < f1_score) {
                f1_score_05 = f1_score;
                th[FOLDING_DTTCM_F1SCORE_05] = ths._[t];
            }
        }
        {
            double f1_score = _f1score_beta(multi, 0.1);
            if (f1_score_01 < f1_score) {
                f1_score_01 = f1_score;
                th[FOLDING_DTTCM_F1SCORE_01] = ths._[t];
            }
        }
    }
    free(ths._);

    for (int t = 0; t < N_THCHOICEMETHODs; t++) {
        _test(ds, fds->test.multi, th[t], test->multi[t]);
    }
}

void folding_tests(Dataset* ds, Folding* folding) {
    for (int32_t k = 0; k < folding->config.kfolds; k++) {
        printf("Split-test for fold %d\n", k);
        folding_test(ds, &folding->items[k]);
    }
}

void folding_generate(Dataset* ds, FoldingConfig config, Folding** folding) {
    const int32_t KFOLDS = config.kfolds;
    (*folding)->items = calloc(KFOLDS, sizeof(FoldingItem));
    folding_datasets(ds, *folding);
    folding_tests(ds, *folding);
}

void folding_free(Folding* folding) {
    for (int32_t k = 0; k < folding->config.kfolds; k++) {
        for (int32_t cl = 0; cl < N_CLASSES; cl++) {
            free(folding->items[k].dataset.test.multi[cl]._);
            free(folding->items[k].dataset.train.multi[cl]._);
        }
    }
    free(folding->items);
    free(folding);
}