
#ifndef __FOLDING_H__
#define __FOLDING_H__

#include "dataset.h"
#include "detect.h"

#define N_THCHOICEMETHODs 5

#define N_FOLDINGDESCRIBEMETRICs 5

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

typedef struct FoldingDetection {
    double score;
    double th;
    Evaluation detection;
} FoldingDetection;

typedef int32_t SourcesNumbers[N_DGACLASSES];
typedef struct FoldingConfig {
    int32_t kfolds;
    
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} FoldingConfig;

typedef struct FoldingTT {
    DatasetRWindows rwindows;
    Evaluations evaluations; // in the train case they are BEST evaluations
} FoldingTT;

typedef struct FoldingK {
    int32_t k;
    FoldingTT train;
    FoldingTT test;
} FoldingK;

typedef struct Folding {
    SourcesNumbers sources_numbers;
    FoldingConfig config;
    FoldingK* ks;
} Folding;

void folding_datasets(Dataset*, Folding*);

void folding_train(FoldingK*);

void folding_test(FoldingK*);

void folding_generate(Dataset* ds, Folding* folding);

void folding_run(Dataset* ds, Folding* folding);

#endif