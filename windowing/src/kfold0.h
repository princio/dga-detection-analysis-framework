#ifndef __KFOLD0_H__
#define __KFOLD0_H__

#include "detect.h"
#include "tt.h"
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
    
    int32_t kfolds;
    
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} KFoldConfig0;

typedef struct KFold0 {
    KFoldConfig0 config;
    PSet* pset;
    MANY(TT0) ks0;
} KFold0;

KFold0 kfold0_run(const Dataset0 ds,  KFoldConfig0 config);

void kfold0_free(KFold0* kfold);

#endif