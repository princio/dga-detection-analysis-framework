#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include "kfold.h"
#include "parameters.h"
#include "sources.h"

typedef struct Experiment {
    char name[100];
    char rootpath[500];

    MANY(PSet) psets;

    MANY(Galaxy) galaxies;
    
} Experiment;

extern Experiment experiment;

#endif