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

void kfold0_splits(MANY(RWindow0) rwindows_src, MANY(TT0) tts, DGAClass cl, KFoldConfig0 config, const int32_t _max_windows) {
    const int32_t KFOLDs = config.kfolds;
    const int32_t TRAIN_KFOLDs = config.kfolds - config.test_folds;
    const int32_t TEST_KFOLDs = config.test_folds;

    const int32_t max_windows = (_max_windows != 0) && (_max_windows < rwindows_src.number) ? _max_windows : rwindows_src.number;

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

        T_PC(MANY(RWindow0)) drw_train = &tts._[k][cl].train;
        T_PC(MANY(RWindow0)) drw_test = &tts._[k][cl].test;

        INITMANYREF(drw_train, train_size, RWindow0);
        INITMANYREF(drw_test, test_size, RWindow0);

        train_index = 0;
        test_index = 0;
        for (int32_t kk = 0; kk < KFOLDs; kk++) {

            int32_t start = kk * kfold_size;
            int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

            if (kindexes[k][kk] == 0) {
                memcpy(&drw_train->_[train_index], &rwindows_src._[start], sizeof(RWindow0) * nwindows);
                train_index += nwindows;
            } else {
                memcpy(&drw_test->_[test_index], &rwindows_src._[start], sizeof(RWindow0) * nwindows);
                test_index += nwindows;
            }
        }
    }
}

KFold0 kfold0_run(const Dataset0 ds, KFoldConfig0 config) {
    assert((config.balance_method != FOLDING_DSBM_EACH) || (config.split_method != FOLDING_DSM_IGNORE_1));

    KFold0 kfold;
    MANY(TT0) tts0;

    if (config.kfolds <= 1 || config.kfolds - 1 < config.test_folds) {
        fprintf(stderr, "Error within kfolds (%d) and test_folds (%d).", config.kfolds, config.test_folds);
        exit(1);
    }

    INITMANY(tts0, config.kfolds, TT0);

    kfold.config = config;
    kfold.ks0 = tts0;

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        MANY(RWindow0) rwindows;

        INITMANY(rwindows, ds.windows.multi[cl].number, RWindow0);

        memcpy(rwindows._, ds.windows.multi[cl]._, sizeof(RWindow0) * ds.windows.multi[cl].number);

        if (config.shuffle) {
            windows0_shuffle(rwindows);
        }

        kfold0_splits(rwindows, tts0, cl, config, 0);

        free(rwindows._);
    }

    return kfold;
}

void kfold0_free(KFold0* kfold) {
    for (int32_t k = 0; k < kfold->config.kfolds; k++) {
        DGAFOR(cl) {
            free(kfold->ks0._[k][cl].train._);
            free(kfold->ks0._[k][cl].test._);
        }
    }

    free(kfold->ks0._);
}