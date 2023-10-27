
#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include "folding.h"
#include "foldingeval.h"
#include "parameters.h"

typedef struct ExperimentFolding {

    FoldingConfig config;
    FoldingEval eval;
    Folding* folds;

} ExperimentFolding;

typedef struct Experiment {
    char name[100];
    char rootpath[500];
    
    PSets psets;

    DatasetSources sources;

    ExperimentFolding folding;
} Experiment;


void experiment_sources_add(Experiment* exp, Source* source);

void experiment_sources_fill(Experiment* exp);


#endif