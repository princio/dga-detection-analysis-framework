#ifndef __WINDOWINGTESTBED2DATASET_H__
#define __WINDOWINGTESTBED2DATASET_H__

#include "common.h"

#include "dataset.h"
#include "tb2w.h"

#include <linux/limits.h>

#define TB2_WSIZES_N(A) A->wsizes.number

typedef struct FoldConfig {
    size_t k;
    size_t k_test;
} FoldConfig;

typedef DatasetSplits RTB2DBy_fold;
MAKEMANY(RTB2DBy_fold);

typedef struct TB2DBy_try {
    RDataset dataset;
    MANY(RTB2DBy_fold) byfold;
} TB2DBy_try;
MAKEMANY(TB2DBy_try);

MAKEMANY(FoldConfig);

typedef struct TB2D {
    RTB2W tb2w;

    MANY(FoldConfig) folds;

    struct {
        const size_t try;
        const size_t fold;
    } n;

    MANY(TB2DBy_try) bytry;
} TB2D;

RTB2D tb2d_generate(RTB2W tb2w, size_t n_try, MANY(FoldConfig) foldconfigs);
void tb2d_free(RTB2D);

#endif