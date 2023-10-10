
#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include "dn.h"

#define MAX_SPLITPERCENTAGES 10




typedef struct ExperimentSet {

    int N_SPLITRATIOs;
    double split_percentages[MAX_SPLITPERCENTAGES];

    int KFOLDs;
    
    WindowingPtr windowing;

    EvaluationMetricFunctions evmfs;
} ExperimentSet;

typedef struct CMAvgCursor {
    int split;
    int kfold;
    int wsize;
    int metric;
} CMAvgCursor;


double evfn_f1score_beta1(Prediction pr);

double evfn_f1score_beta05(Prediction pr);

double evfn_f1score_beta01(Prediction pr);

double evfn_fpr(Prediction pr);

double evfn_tpr(Prediction pr);



WindowingPtr experiment_run(Experiment);

void experiment_test(ExperimentSet* es);

#endif