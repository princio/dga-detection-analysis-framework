
#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include "parameters.h"
#include "sources.h"


typedef struct ExperimentFolding {

    DatasetFoldsConfig config;
    DatasetFolds folds;
    DatasetFoldsDescribe describe;

} ExperimentFolding;

typedef struct Experiment {
    char name[100];
    char rootpath[500];
    
    PSets psets;

    SourcesLists sources_lists;
    SourcesArrays sources_arrays;

    ExperimentFolding folding;
} Experiment;


void experiment_sources_add(Experiment* exp, Source* source);

void experiment_sources_fill(Experiment* exp);


#endif