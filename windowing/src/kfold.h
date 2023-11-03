
#ifndef __KFOLD_H__
#define __KFOLD_H__

#include "dataset.h"
#include "detect.h"
#include "tt.h"

char thcm_names[5][20];
typedef enum FoldingTestConfigSplitBalanceMethod {
    FOLDING_DSBM_EACH,
    FOLDING_DSBM_NOT_INFECTED
} FoldingTestConfigSplitBalanceMethod;

typedef enum FoldingTestConfigSplitMethod {
    FOLDING_DSM_IGNORE_1,
    FOLDING_DSM_MERGE_12
} FoldingTestConfigSplitMethod;

// typedef struct DatasetSplitConfig {
//     FoldingTestConfigSplitBalanceMethod dsbm;
//     FoldingTestConfigSplitMethod dsm;
// } DatasetSplitConfig;

typedef struct KFoldConfig {
    int32_t kfolds;
    
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} KFoldConfig;

typedef struct KFoldSet {
    int32_t k;
    TT tt;
    MANY(TTEvaluation) evaluations;
} KFoldSet;

MAKEMANY(KFoldSet);

typedef struct KFold {
    KFoldConfig config;
    MANY(KFoldSet) ks;
} KFold;


MANY(TT) kfold_sets(Dataset* ds, KFoldConfig config);

MANY(KFoldSet) kfold_evaluation(MANY(TT) tts, MANY(Performance) performances);

KFold kfold_run(Dataset* ds, KFoldConfig config, MANY(Performance) performances);

void kfold_free(KFold kfold);

#endif