#include "kfold.h"

#include "dataset.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define FALSERATIO(tf) (((double) (tf).falses) / ((tf).falses + (tf).trues))
#define TRUERATIO(tf) (((double) (tf).trues) / ((tf).falses + (tf).trues))


#define MIN(A, B) (A.min) = ((B) <= (A.min) ? (B) : (A.min))
#define MAX(A, B) (A.max) = ((B) >= (A.max) ? (B) : (A.max))


MANY(TT) kfold_sets(Dataset* ds, KFoldConfig config) {
    assert((config.balance_method != FOLDING_DSBM_EACH) || (config.split_method != FOLDING_DSM_IGNORE_1));

    if (config.kfolds <= 1 || config.kfolds - 1 < config.test_folds) {
        fprintf(stderr, "Error within kfolds (%d) and test_folds (%d).", config.kfolds, config.test_folds);
        exit(1);
    }

    MANY(TT) tts;

    tts.number = config.kfolds;
    tts._ = calloc(config.kfolds, sizeof(TT));

    MANY(RWindow) rwindows[N_DGACLASSES];

    dataset_rwindows(ds, rwindows, config.shuffle);

    const int32_t KFOLDs = config.kfolds;
    const int32_t TRAIN_KFOLDs = config.kfolds - config.test_folds;
    const int32_t TEST_KFOLDs = config.test_folds;

    int32_t kindexes[KFOLDs][KFOLDs];
    for (int32_t k = 0; k < KFOLDs; k++) {
        for (int32_t kindex = k; kindex < (k + TRAIN_KFOLDs); kindex++) {
            kindexes[k][kindex % KFOLDs] = 0;
        }
        for (int32_t kindex = k + TRAIN_KFOLDs; kindex < (k + KFOLDs); kindex++) {
            kindexes[k][kindex % KFOLDs] = 1;
        }
    }


    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        int32_t kfold_size = rwindows[cl].number / KFOLDs;
        int32_t kfold_size_rest = rwindows[cl].number - (kfold_size * KFOLDs);

        for (int32_t k = 0; k < KFOLDs; k++) {

            int32_t train_size;
            int32_t test_size;

            train_size = kfold_size * TRAIN_KFOLDs;
            test_size = kfold_size * TEST_KFOLDs;

            MANY(RWindow)* drw_train = tts._[k].train;
            MANY(RWindow)* drw_test = tts._[k].test;

            if (kindexes[k][KFOLDs - 1]) {
                test_size += kfold_size_rest;
            } else {
                train_size += kfold_size_rest;
            }

            INITMANY(drw_train[cl], train_size, RWindow);
            INITMANY(drw_test[cl], test_size, RWindow);

            int32_t train_index = 0;
            int32_t test_index = 0;
            for (int32_t kk = 0; kk < KFOLDs; kk++) {

                int32_t start = kk * kfold_size;
                int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&drw_train[cl]._[train_index], &rwindows[cl]._[start], sizeof(Window*) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&drw_test[cl]._[test_index], &rwindows[cl]._[start], sizeof(Window*) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        free(rwindows[cl]._);
    }

    return tts;
}

MANY(KFoldSet) kfold_evaluation(MANY(TT) tts, MANY(Performance) performances) {
    MANY(KFoldSet) ks;

    ks.number = tts.number;
    ks._ = calloc(tts.number, sizeof(KFoldSet));

    for (int32_t k = 0; k < tts.number; k++) {
        ks._[k].tt = tts._[k];
        ks._[k].evaluations = tt_run(&tts._[k], performances);
    }

    free(tts._);

    return ks;
}

KFold kfold_run(Dataset* ds, KFoldConfig config, MANY(Performance) performances) {
    MANY(TT) tts = kfold_sets(ds, config);
    MANY(KFoldSet) ks = kfold_evaluation(tts, performances);

    KFold kfold;

    kfold.config = config;
    kfold.ks = ks;

    return kfold;
}

void kfold_free(KFold kfold) {
    for (int32_t k = 0; k < kfold.config.kfolds; k++) {
        tt_free(kfold.ks._[k].tt);
        tt_evaluations_free(kfold.ks._[k].evaluations);
    }
    free(kfold.ks._);
}
