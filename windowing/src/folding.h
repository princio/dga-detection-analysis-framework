
#ifndef __FOLDING_H__
#define __FOLDING_H__

#include "dataset.h"
#include "experiment.h"

#define N_THCHOICEMETHODs 5

char thcm_names[5][20];

#define FALSERATIO(tf) (((double) (tf).all.falses) / ((tf).all.falses + (tf).all.trues))
#define TRUERATIO(tf) (((double) (tf).all.trues) / ((tf).all.falses + (tf).all.trues))




typedef enum FoldingTestConfigSplitBalanceMethod {
    FOLDING_DSBM_EACH,
    FOLDING_DSBM_NOT_INFECTED
} FoldingTestConfigSplitBalanceMethod;



typedef enum FoldingTestConfigSplitMethod {
    FOLDING_DSM_IGNORE_1,
    FOLDING_DSM_MERGE_12
} FoldingTestConfigSplitMethod;



typedef enum FoldingTestThresoldChoiceMethod {
    FOLDING_DTTCM_F1SCORE,
    FOLDING_DTTCM_F1SCORE_05,
    FOLDING_DTTCM_F1SCORE_01,
    FOLDING_DTTCM_FPR, // il train non ha senso sugli infetti
    FOLDING_DTTCM_TPR  // il train non ha senso sui non infetti
} FoldingTestThresoldChoiceMethod;

typedef struct DatasetTrainTest {

    int32_t wsize;

    double percentage_split; // train over test

    WindowRefs train[N_CLASSES];
    WindowRefs test[N_CLASSES];

} DatasetTrainTest;

typedef struct DatasetSplitConfig {
    FoldingTestConfigSplitBalanceMethod dsbm;
    FoldingTestConfigSplitMethod dsm;
} DatasetSplitConfig;

typedef struct DatasetWindows {
    WindowRefs single[2];
    WindowRefs multi[N_CLASSES];
} DatasetWindows;

typedef struct FoldingDataset {
    int32_t kfold;

    Sources sources;

    DatasetWindows train;
    DatasetWindows test;
} FoldingDataset;

typedef struct FoldingDatasets {
    int32_t number;
    FoldingDataset* _;
} FoldingDatasets;

typedef struct FoldingConfig {
    int32_t kfolds;
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;

    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
    
} FoldingConfig;


typedef struct FoldingConfig {
    int32_t kfolds;
    
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} FoldingConfig;

typedef struct TF {
    int32_t falses;
    int32_t trues;
} TF;

typedef struct Test {
    TF windows;
    TF* sources; // number of trues/falses windows for each source
} Test;


typedef struct FoldingTest {
    double ths[N_THCHOICEMETHODs];
    Test single[2];
    Test multi[N_THCHOICEMETHODs][N_CLASSES];
} FoldingTest;

typedef struct FoldingTests {
    int32_t number;
    FoldingTest* _;
} FoldingTests;

typedef struct FoldingsDescribeItems {
    
    double min;
    double max;
    double avg;
    
} FoldingsDescribeItems;

typedef struct FoldingsDescribe {

    FoldingsDescribeItems th[N_THCHOICEMETHODs];

    FoldingsDescribeItems falses[N_THCHOICEMETHODs][N_CLASSES];
    FoldingsDescribeItems trues[N_THCHOICEMETHODs][N_CLASSES];

    FoldingsDescribeItems false_ratio[N_THCHOICEMETHODs][N_CLASSES];
    FoldingsDescribeItems true_ratio[N_THCHOICEMETHODs][N_CLASSES];

    FoldingsDescribeItems sources_trues[N_THCHOICEMETHODs][N_CLASSES];

} FoldingsDescribe;


typedef struct FoldingItem {
    FoldingDataset dataset;
    FoldingTest test;
} FoldingItem;


typedef struct Folding {
    FoldingConfig config;
    FoldingItem* items;
} Folding;


typedef struct Foldings {
    int32_t number;
    Folding* _;
} Foldings;

void folding_datasets(Dataset*, Folding*);

void folding_test(Dataset*, FoldingItem*);

void folding_tests(Dataset*, Folding*);

void folding_generate(Dataset*, FoldingConfig, Folding**);

#endif