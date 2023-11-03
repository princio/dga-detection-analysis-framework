
#ifndef __DATASET_H__
#define __DATASET_H__

#include "common.h"

#include "parameters.h"
#include "sources.h"
#include "windows.h"

MAKEDGAMANY(Window);
MAKEDGAMANY(RWindow);

typedef struct DatasetSources {
    DGA(SourcesList) lists;
    DGA(SourcesArray) arrays;
} DatasetSources;

typedef struct Dataset {
    int32_t id;
    PSet* pset;
    DatasetSources sources;
    DGAMANY(Window) windows;
} Dataset;

MAKEMANY(Dataset);

void dataset_rwindows(Dataset*, DGAMANY(RWindow), int32_t);

#endif