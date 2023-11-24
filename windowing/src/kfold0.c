#include "kfold0.h"

#include "cache.h"
#include "io.h"
#include "windows0.h"

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

void kfold0_splits(MANY(RWindow0) windows0, MANY(TT0)* tts, DGAClass cl, KFoldConfig0 config, const size_t _max_windows) {
    const int32_t KFOLDs = config.kfolds;
    const int32_t TRAIN_KFOLDs = config.kfolds - config.test_folds;
    const int32_t TEST_KFOLDs = config.test_folds;

    const size_t max_windows = (_max_windows != 0) && (_max_windows < windows0.number) ? _max_windows : windows0.number;

    const int32_t kfold_size = max_windows / KFOLDs;
    const int32_t kfold_size_rest = max_windows - (kfold_size * KFOLDs);

    int32_t kindexes[KFOLDs][KFOLDs]; {
        for (int32_t k = 0; k < KFOLDs; k++) {
            for (int32_t kindex = k; kindex < (k + TRAIN_KFOLDs); kindex++) {
                kindexes[k][kindex % KFOLDs] = 0;
            }
            for (int32_t kindex = k + TRAIN_KFOLDs; kindex < (k + KFOLDs); kindex++) {
                kindexes[k][kindex % KFOLDs] = 1;
            }
        }
    }

    for (int32_t k = 0; k < KFOLDs; k++) {
        
        int32_t train_index;
        int32_t test_index;

        const int32_t train_size = kfold_size * TRAIN_KFOLDs + (!kindexes[k][KFOLDs - 1] ? kfold_size_rest : 0);
        const int32_t test_size = kfold_size * TEST_KFOLDs + (kindexes[k][KFOLDs - 1] ? kfold_size_rest : 0);

        MANY(RWindow0)* train = &tts->_[k][cl].train;
        MANY(RWindow0)* test = &tts->_[k][cl].test;

        INITMANYREF(train, train_size, RWindow0);
        INITMANYREF(test, test_size, RWindow0);

        train_index = 0;
        test_index = 0;
        for (int32_t kk = 0; kk < KFOLDs; kk++) {

            int32_t start = kk * kfold_size;
            int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

            if (kindexes[k][kk] == 0) {
                memcpy(&train->_[train_index], &windows0._[start], sizeof(RWindow0) * nwindows);
                train_index += nwindows;
            } else {
                memcpy(&test->_[test_index], &windows0._[start], sizeof(RWindow0) * nwindows);
                test_index += nwindows;
            }
        }
    }
}

KFold0 kfold0_run(const Dataset0 ds, KFoldConfig0 config) {
    assert((config.balance_method != FOLDING_DSBM_EACH) || (config.split_method != FOLDING_DSM_IGNORE_1));

    KFold0 kfold;
    memset(&kfold, 0, sizeof(KFold0));

    if (config.kfolds <= 1 || config.kfolds - 1 < config.test_folds) {
        fprintf(stderr, "Error within kfolds (%d) and test_folds (%d).", config.kfolds, config.test_folds);
        exit(1);
    }

    kfold.config = config;
    INITMANY(kfold.ks0, config.kfolds, TT0);

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        MANY(RWindow0) rwindows;

        INITMANY(rwindows, ds.windows[cl].number, RWindow0);

        memcpy(rwindows._, ds.windows[cl]._, sizeof(RWindow0) * ds.windows[cl].number);

        if (config.shuffle) {
            // windows0_shuffle(rwindows);
        }

        kfold0_splits(rwindows, &kfold.ks0, cl, config, 0);

        free(rwindows._);
    }

    return kfold;
}

void kfold0_free(KFold0* kfold) {
    for (int32_t k = 0; k < kfold->config.kfolds; k++) {
        DGAFOR(cl) {
            FREEMANY(kfold->ks0._[k][cl].train);
            FREEMANY(kfold->ks0._[k][cl].test);
        }
    }

    free(kfold->ks0._);
}