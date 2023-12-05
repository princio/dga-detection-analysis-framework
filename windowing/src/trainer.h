
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

typedef struct ResultsKFold0s {
    MANY(KFold0) wsize;
} ResultsKFold0s;

typedef struct Results {
    TestBed2* tb2;

    MANY(Performance) thchoosers;
    ResultsKFold0s kfolds;
    MANY(ResultsWSize) wsize;
} Results;

#define RESULT_IDX(R, WSIZE, APPLY, THCHOOSER, SPLIT) R.wsize._[WSIZE].apply._[APPLY].thchooser._[THCHOOSER].kfold._[SPLIT]

Results* trainer_run(TestBed2*tb2, MANY(Performance) thchoosers, KFoldConfig0);

void trainer_free(Results* results);

void results_io(IOReadWrite rw, char dirname[200], Results* results);

#endif