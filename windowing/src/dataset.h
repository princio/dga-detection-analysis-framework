
#ifndef __DATASET_H__
#define __DATASET_H__


#include "dn.h"

#define N_THCHOICEMETHODs 5

char thcm_names[5][20];

#define FALSERATIO(tf) (((double) (tf).falses) / ((tf).falses + (tf).trues))
#define TRUERATIO(tf) (((double) (tf).trues) / ((tf).falses + (tf).trues))


typedef enum DatasetSplitBalanceMethod {
    DSBM_EACH,
    DSBM_NOT_INFECTED
} DatasetSplitBalanceMethod;


typedef enum DatasetSplitMethod {
    DSM_IGNORE_1,
    DSM_MERGE_12
} DatasetSplitMethod;


typedef enum DatasetTestThresoldChoiceMethod {
    DTTCM_F1SCORE,
    DTTCM_F1SCORE_05,
    DTTCM_F1SCORE_01,
    DTTCM_FPR, // il train non ha senso sugli infetti
    DTTCM_TPR  // il train non ha senso sui non infetti
} DatasetTestThresoldChoiceMethod;


typedef struct DatasetSplitConfig {
    DatasetSplitBalanceMethod dsbm;
    DatasetSplitMethod dsm;
} DatasetSplitConfig;

typedef struct DatasetWindows {
    WindowRefs single[2];
    WindowRefs multi[N_CLASSES];
} DatasetWindows;

typedef struct DatasetSplit {
    int32_t kfold;

    DatasetWindows train;
    DatasetWindows test;
} DatasetSplit;

typedef struct DatasetFoldsConfig {
    int32_t kfolds;
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    DatasetSplitBalanceMethod balance_method;
    DatasetSplitMethod split_method;
} DatasetFoldsConfig;

typedef struct DatasetFolds {
    int32_t kfolds;
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    DatasetSplitBalanceMethod balance_method;
    DatasetSplitMethod split_method;

    DatasetSplit* splits;
} DatasetFolds;

typedef struct TF {
    int32_t falses;
    int32_t trues;
} TF;


typedef struct DatasetTest {
    double ths[N_THCHOICEMETHODs];
    TF single[2];
    TF multi[N_THCHOICEMETHODs][N_CLASSES];
} DatasetTest;

typedef struct DatasetTests {
    int32_t number;
    DatasetTest* _;
} DatasetTests;

typedef struct DatasetFoldsDescribeItems {
    
    double min;
    double max;
    double avg;
    
} DatasetFoldsDescribeItems;

typedef struct DatasetFoldsDescribe {

    DatasetFoldsDescribeItems th[N_THCHOICEMETHODs];

    DatasetFoldsDescribeItems falses[N_THCHOICEMETHODs][N_CLASSES];
    DatasetFoldsDescribeItems trues[N_THCHOICEMETHODs][N_CLASSES];

    DatasetFoldsDescribeItems false_ratio[N_THCHOICEMETHODs][N_CLASSES];
    DatasetFoldsDescribeItems true_ratio[N_THCHOICEMETHODs][N_CLASSES];

} DatasetFoldsDescribe;


void dataset_traintestsplit(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split, DatasetSplitMethod dsm);

void dataset_traintestsplit_cm(int32_t wsize, PSets* psets, DatasetTrainTestPtr dt_tt);

void dataset_folds_free(DatasetFolds* df);

void dataset_folds(Dataset0* dt, DatasetFolds*);

DatasetTests dataset_foldstest(Dataset0* dt, DatasetFoldsConfig df_config);

#endif