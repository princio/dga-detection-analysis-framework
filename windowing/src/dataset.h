
#ifndef __DATASET_H__
#define __DATASET_H__

#include "parameters.h"
#include "sources.h"
#include "windows.h"

typedef Windows DatasetWindows[N_DGACLASSES];
typedef RWindows DatasetRWindows[N_DGACLASSES];

typedef SourcesList DatasetSourcesLists[N_DGACLASSES];
typedef SourcesArray DatasetSourcesArrays[N_DGACLASSES];

typedef struct DatasetSources {
    DatasetSourcesLists lists;
    DatasetSourcesArrays arrays;
} DatasetSources;

typedef struct Dataset {
    int32_t id;
    PSet* pset;
    DatasetSources sources;
    DatasetWindows windows;
} Dataset;

typedef struct Datasets {
    int32_t number;
    Dataset* _;
} Datasets;

void dataset_rwindows(Dataset*, DatasetRWindows, int32_t);

void dataset_rwindows_ths(DatasetRWindows dsrwindows, Ths* ths);

#endif