
#ifndef __DATASET_H__
#define __DATASET_H__

#include "parameters.h"


typedef struct SourcesLists {
    SourcesList binary;
    SourcesList multi[N_CLASSES];
} SourcesLists;


typedef struct SourcesArrays {
    SourcesArray binary;
    SourcesArray multi[N_CLASSES];
} SourcesArrays;


typedef struct DatasetSources {
    SourcesLists* lists;
    SourcesArrays* arrays;
} DatasetSources;


typedef struct Dataset {
    int32_t id;

    PSet* pset;

    DatasetSources sources;

    Windows windows[N_CLASSES];

    WindowRefs windows_all;
} Dataset;


typedef struct Datasets {
    int32_t number;
    Dataset* _;
} Datasets;


#endif