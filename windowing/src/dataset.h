
#ifndef __DATASET_H__
#define __DATASET_H__

#include "common.h"
#include "windowmany.h"

typedef struct DatasetRWindow {
    MANY(RWindow) all;
    MANY(RWindow) binary[2];
    MANY(RWindow) multi[N_DGACLASSES];
} DatasetRWindow;

typedef struct __Dataset {
    WSize wsize;
    MANY(MinMax) minmax;
    DatasetRWindow windows;
} __Dataset;

typedef struct DatasetSplit {
    RWindowMC train;
    RWindowMC test;
} DatasetSplit;

MAKEMANY(DatasetSplit);

typedef struct DatasetSplits {
    int isok;
    MANY(DatasetSplit) splits;
} DatasetSplits;


void dataset_minmax(RWindowMC ds);

RWindowMC dataset_alloc();
RWindowMC dataset_create(WSize wsize, IndexMC counter);
void dataset_shuffle(RWindowMC dataset);
int dataset_splits_ok(MANY(DatasetSplit) splits);
DatasetSplits dataset_splits(RWindowMC dataset, const size_t k, const size_t k_test);
IndexMC dataset_counter(RWindowMC ds);
RWindowMC dataset_from_windowings(MANY(RWindowing) windowings);

#endif