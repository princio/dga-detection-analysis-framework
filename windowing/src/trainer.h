
#ifndef __TRAINER_H__
#define __TRAINER_H__

#include "common.h"

#include "kfold0.h"
#include "window0s.h"

typedef struct Result {
    Performance* threshold_chooser;
    Detection best_train;
    Detection train_test;
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

typedef struct Results {
    MANY(WSize) wsizes;
    MANY(PSet) applies;
    MANY(Performance) thchoosers;
    MANY(KFold0) kfolds;

    MANY(ResultsWSize) wsize;
} Results;

#define RESULT_IDX(R, WSIZE, APPLY, THCHOOSER, SPLIT) R.wsize._[WSIZE].apply._[APPLY].thchooser._[THCHOOSER].kfold._[SPLIT]

Results trainer_run(TestBed2*tb2, MANY(Performance) thchoosers, KFoldConfig0);

void trainer_free(Results* results);

#endif