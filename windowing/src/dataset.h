
#ifndef __DATASET_H__
#define __DATASET_H__

#include "common.h"
#include "window0s.h"

typedef struct DatasetRWindow0 {
    MANY(RWindow0) all;
    MANY(RWindow0) binary[2];
    MANY(RWindow0) multi[N_DGACLASSES];
} DatasetRWindow0;

typedef struct __Dataset0 {
    WSize wsize;
    DatasetRWindow0 windows;
} __Dataset0;

typedef struct DatasetSplit0 {
    RDataset0 train;
    RDataset0 test;
} DatasetSplit0;

MAKEMANY(DatasetSplit0);

typedef struct DatasetSplits {
    int isok;
    MANY(DatasetSplit0) splits;
} DatasetSplits;

RDataset0 dataset0_alloc();
RDataset0 dataset0_create(WSize wsize, Index counter);
int dataset0_splits_ok(MANY(DatasetSplit0) splits);
DatasetSplits dataset0_splits(RDataset0 dataset0, const size_t k, const size_t k_test);
Index dataset_counter(RDataset0 ds);
RDataset0 dataset0_from_windowings(MANY(RWindowing) windowings);

#endif