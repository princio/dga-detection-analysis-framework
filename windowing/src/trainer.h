
#ifndef __TRAINER_H__
#define __TRAINER_H__

#include "common.h"

#include "window0s.h"
#include "testbed2.h"

typedef struct Result {
    Performance* threshold_chooser;
    Detection best_train;
    Detection best_test;
} Result;

MAKEMANY(Result);

typedef struct ResultsFoldTriesSplits {
    MANY(Result) bythchooser;
} ResultsFoldTriesSplits;

MAKEMANY(ResultsFoldTriesSplits);

typedef struct ResultsFoldTries {
    MANY(ResultsFoldTriesSplits) bysplits;
} ResultsFoldTries;

MAKEMANY(ResultsFoldTries);

typedef struct ResultsFold {
    MANY(ResultsFoldTries) bytry;
} ResultsFold;

MAKEMANY(ResultsFold);

typedef struct ResultsApply {
    MANY(ResultsFold) byfold;
} ResultsApply;

MAKEMANY(ResultsApply);

typedef struct ResultsWSize {
    MANY(ResultsApply) byapply;
} ResultsWSize;

MAKEMANY(ResultsWSize);

typedef struct TrainerResults {
    MANY(ResultsWSize) bywsize;
} TrainerResults;

typedef struct __Trainer {
    RTestBed2 tb2;
    MANY(Performance) thchoosers;

    TrainerResults results;
} __Trainer;

typedef struct __Trainer* RTrainer;

#define RESULT_IDX(R, WSIZE, APPLY, FOLD, TRY, SPLIT, THCHOOSER) R.bywsize._[WSIZE].byapply._[APPLY].byfold._[FOLD].bytry._[TRY].bysplits._[SPLIT].bythchooser._[THCHOOSER]

RTrainer trainer_run(RTestBed2, MANY(Performance));

void trainer_free(RTrainer results);

void trainer_io(IOReadWrite rw, char dirname[200], RTestBed2, RTrainer* results);

#endif