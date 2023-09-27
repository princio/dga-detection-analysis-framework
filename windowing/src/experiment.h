
#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include "dn.h"

#define MAX_SPLITPERCENTAGES 10

typedef struct ExperimentSet {

    int N_SPLITRATIOs;
    double split_percentages[MAX_SPLITPERCENTAGES];

    int KFOLDs;
    
    WindowingPtr windowing;
} ExperimentSet;

typedef struct CMAvgCursor {
    int split;
    int kfold;
    int wsize;
    int metric;
} CMAvgCursor;

#endif