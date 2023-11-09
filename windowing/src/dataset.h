
#ifndef __DATASET_H__
#define __DATASET_H__

#include "common.h"

#include "parameters.h"
#include "sources.h"
#include "windows.h"

typedef struct Dataset {
    int32_t id;
    PSet* pset;
    DGAMANY(Window) windows;
} Dataset;

MAKEMANY(Dataset);

void dataset_rwindows(Dataset*, DGAMANY(RWindow), int32_t);

#endif