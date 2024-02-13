
#ifndef __DATASET_H__
#define __DATASET_H__

#include "common.h"
#include "windows.h"

typedef struct DatasetRWindow {
    MANY(RWindow) all;
    MANY(RWindow) binary[2];
    MANY(RWindow) multi[N_DGACLASSES];
    MANY(RWindow) multi_sorted[N_DGACLASSES];
} DatasetRWindow;

typedef struct __Dataset {
    size_t n_configs;
    WSize wsize;
    MANY(MinMax) minmax;
    DatasetRWindow windows;
} __Dataset;

typedef struct DatasetSplit {
    RDataset train;
    RDataset test;
} DatasetSplit;

MAKEMANY(DatasetSplit);

typedef struct DatasetSplits {
    int isok;
    MANY(DatasetSplit) splits;
} DatasetSplits;


void dataset_minmax(RDataset ds);

RDataset dataset_alloc();
RDataset dataset_create(WSize wsize, Index counter, const size_t n_configs);
void dataset_shuffle(RDataset dataset);
int dataset_splits_ok(MANY(DatasetSplit) splits);
DatasetSplits dataset_splits(RDataset dataset, const size_t k, const size_t k_test);
Index dataset_counter(RDataset ds);
RDataset dataset_from_windowings(MANY(RWindowing) windowings, const size_t n_configs);

#endif