#include "kfold0.h"

#include "cache.h"
#include "dataset.h"
#include "io.h"
#include "window0s.h"

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

KFold0 kfold0_run(const RDataset0 ds, KFoldConfig0 config) {
    assert((config.balance_method != FOLDING_DSBM_EACH) || (config.split_method != FOLDING_DSM_IGNORE_1));

    KFold0 kfold;
    memset(&kfold, 0, sizeof(KFold0));

    if (config.kfolds <= 1 || config.kfolds - 1 < config.test_folds) {
        fprintf(stderr, "Error within kfolds (%ld) and test_folds (%ld).", config.kfolds, config.test_folds);
        exit(1);
    }

    kfold.config = config;
    kfold.splits = dataset0_split_k(ds, config.kfolds, config.test_folds);

    return kfold;
}

int kfold0_ok(KFold0* kfold) {
    return (kfold->splits.number == 0 && kfold->splits._ == 0x0) == 0;
}

void kfold0_free(KFold0* kfold) {
    if (kfold0_ok(kfold)) {
        FREEMANY(kfold->splits);
    }
}