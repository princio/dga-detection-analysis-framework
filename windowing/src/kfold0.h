#ifndef __KFOLD0_H__
#define __KFOLD0_H__

#include "dataset.h"
#include "detect.h"
#include "testbed2.h"

typedef enum FoldingTestConfigSplitBalanceMethod {
    FOLDING_DSBM_EACH,
    KFOLD_BM_NOT_INFECTED
} FoldingTestConfigSplitBalanceMethod;

typedef enum FoldingTestConfigSplitMethod {
    FOLDING_DSM_IGNORE_1,
    KFOLD_SM_MERGE_12
} FoldingTestConfigSplitMethod;

typedef struct KFoldConfig0 {
    size_t kfolds;
    size_t test_folds; // usually kfold - 1
    int32_t shuffle;
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} KFoldConfig0;

typedef struct KFold0WSizeSplits {
    MANY(DatasetSplit0) splits;
} KFold0WSizeSplits;

MAKEMANY(KFold0WSizeSplits);

typedef struct __KFold0 {
    RTestBed2 testbed2;
    KFoldConfig0 config;
    struct {
        MANY(KFold0WSizeSplits) wsize;
    } datasets;
} __KFold0;

typedef __KFold0* RKFold0;

RKFold0 kfold0_run(RTestBed2 tb2, KFoldConfig0 config);

void kfold0_free(RKFold0 kfold);

void kfold0_io(IOReadWrite rw, char dirname[200], RTestBed2 tb2, RKFold0* kfold0);

#endif