
#ifndef __TRAINER_H__
#define __TRAINER_H__

#include "common.h"

#include "kfold0.h"
#include "window0s.h"

typedef struct Result {
    Performance* threshold_chooser;
    Detection best_train;
    Detection best_test;
} Result;

MAKEMANY(Result);

typedef struct ResultsThChooser {
    MANY(Result) kfold;
} ResultsThChooser;

MAKEMANY(ResultsThChooser);

typedef struct ResultsApply {
    MANY(ResultsThChooser) thchooser;
} ResultsApply;

MAKEMANY(ResultsApply);

typedef struct ResultsWSize {
    MANY(ResultsApply) apply;
} ResultsWSize;

MAKEMANY(ResultsWSize);

typedef struct TrainerResults {
    MANY(ResultsWSize) wsize;
} TrainerResults;

typedef struct __Trainer {
    RKFold0 kfold0;
    MANY(Performance) thchoosers;

    TrainerResults results;
} __Trainer;

typedef struct __Trainer* RTrainer;

#define RESULT_IDX(R, WSIZE, APPLY, THCHOOSER, SPLIT) R.wsize._[WSIZE].apply._[APPLY].thchooser._[THCHOOSER].kfold._[SPLIT]

RTrainer trainer_run(RKFold0, MANY(Performance));

void trainer_free(RTrainer results);

void trainer_io(IOReadWrite rw, char dirname[200], RKFold0 kfold0, RTrainer* results);

#endif