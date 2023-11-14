
#ifndef __DATASET_H__
#define __DATASET_H__

#include "common.h"

#include "windows.h"

typedef MANY(RWindow) Dataset[N_DGACLASSES];

Index dataset_count_sources(Dataset);

#endif