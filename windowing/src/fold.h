#ifndef __FOLD_H__
#define __FOLD_H__

#include "dataset.h"
#include "detect.h"

typedef struct FoldTryWSize {
    DatasetSplits splits;
} FoldTryWSize;

MAKEMANY(FoldTryWSize);

typedef struct FoldTry {
    int isok;
    MANY(FoldTryWSize) bywsize;
} FoldTry;

MAKEMANY(FoldTry);

typedef struct FoldConfig {
    size_t tries;
    size_t k;
    size_t k_test;
} FoldConfig;

typedef struct __Fold {
    RTestBed2 tb2;
    FoldConfig config;
    MANY(FoldTry) tries;
} __Fold;

typedef __Fold* RFold;

MAKEMANY(RFold);

void fold_add(RTestBed2 tb2, FoldConfig config);
void fold_io(IOReadWrite rw, FILE* file, RTestBed2 tb2, RFold* fold);
void fold_md(FILE* file, const RFold fold);

#endif