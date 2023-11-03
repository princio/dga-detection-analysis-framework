
#ifndef __TT_H__
#define __TT_H__

#include "common.h"
#include "detect.h"

typedef struct TT {
    DGAMANY(RWindow) train;
    DGAMANY(RWindow) test;
} TT;

typedef struct Evaluation {
    Detection detection;
    double score;
} Evaluation;

typedef struct TTEvaluation {
    Performance* performance;

    Evaluation train;
    Evaluation test;
} TTEvaluation;

MAKEMANY(TT);
MAKEMANY(TTEvaluation);

void tt_evaluations_init(MANY(Performance), MANY(TTEvaluation)*);
void tt_evaluations_free(MANY(TTEvaluation));


MANY(TTEvaluation) tt_run(TT*, MANY(Performance));
void tt_free(TT);

#endif