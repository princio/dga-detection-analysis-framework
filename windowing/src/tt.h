
#ifndef __TT_H__
#define __TT_H__

#include "common.h"
#include "dataset.h"
#include "detect.h"


typedef struct Evaluation {
    Detection detection;
    double score;
} Evaluation;

typedef struct TTEvaluation {
    Performance* performance;

    Evaluation train;
    Evaluation test;
} TTEvaluation;

MAKEMANY(TTEvaluation);

typedef RWindowSplit TT[N_DGACLASSES];

MAKEMANY(TT);

#endif