#ifndef __KFOLD0_H__
#define __KFOLD0_H__

#include "dataset.h"
#include "detect.h"
#include "testbed2.h"


char thcm_names[5][20];

typedef enum FoldingTestConfigSplitBalanceMethod {
    FOLDING_DSBM_EACH,
    KFOLD_BM_NOT_INFECTED
} FoldingTestConfigSplitBalanceMethod;

typedef enum FoldingTestConfigSplitMethod {
    FOLDING_DSM_IGNORE_1,
    KFOLD_SM_MERGE_12
} FoldingTestConfigSplitMethod;

typedef struct KFoldConfig0 {
    TestBed2* testbed;
    
    size_t kfolds;
    size_t test_folds; // usually kfold - 1

    int32_t shuffle;
    
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} KFoldConfig0;

typedef struct KFold0 {
    KFoldConfig0 config;
    MANY(DatasetSplit0) splits;
} KFold0;

typedef KFold0* RKFold0;

KFold0 kfold0_run(RDataset0 ds,  KFoldConfig0 config);

int kfold0_ok(KFold0* kfold);

void kfold0_free(KFold0* kfold);

#endif