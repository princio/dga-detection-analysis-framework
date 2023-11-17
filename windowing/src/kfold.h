
#ifndef __KFOLD_H__
#define __KFOLD_H__

#include "detect.h"
#include "tt.h"
#include "testbed.h"

char thcm_names[5][20];
typedef enum FoldingTestConfigSplitBalanceMethod {
    FOLDING_DSBM_EACH,
    KFOLD_BM_NOT_INFECTED
} FoldingTestConfigSplitBalanceMethod;

typedef enum FoldingTestConfigSplitMethod {
    FOLDING_DSM_IGNORE_1,
    KFOLD_SM_MERGE_12
} FoldingTestConfigSplitMethod;

// typedef struct DatasetSplitConfig {
//     FoldingTestConfigSplitBalanceMethod dsbm;
//     FoldingTestConfigSplitMethod dsm;
// } DatasetSplitConfig;

typedef struct KFoldConfig {
    TestBed* testbed;
    
    int32_t kfolds;
    
    int32_t test_folds; // usually kfold - 1
    int32_t shuffle;
    
    FoldingTestConfigSplitBalanceMethod balance_method;
    FoldingTestConfigSplitMethod split_method;
} KFoldConfig;

typedef struct KFold {
    KFoldConfig config;
    PSet* pset;
    MANY(TT) ks;
} KFold;

KFold kfold_run(const Dataset ds,  KFoldConfig config, PSet* pset);

void kfold_free(KFold* kfold);

void kfold_io(IOReadWrite rw, FILE* file, void* obj);

void kfold_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]);

void kfold_io_txt(TCPC(void) obj);

#endif