
#ifndef __TRAINER_H__
#define __TRAINER_H__

#include "common.h"

#include "window0s.h"
#include "testbed2.h"

#include <linux/limits.h>

typedef struct TrainerBy_thchooser {
    Performance* threshold_chooser;
    Detection best_train;
    Detection best_test;
} TrainerBy_thchooser;

MAKEMANY(TrainerBy_thchooser);

typedef struct TrainerBy_splits {
    MANY(TrainerBy_thchooser) bythchooser;
} TrainerBy_splits;

MAKEMANY(TrainerBy_splits);

typedef struct TrainerBy_try {
    MANY(TrainerBy_splits) bysplits;
} TrainerBy_try;

MAKEMANY(TrainerBy_try);

typedef struct TrainerBy_fold {
    MANY(TrainerBy_try) bytry;
} TrainerBy_fold;

MAKEMANY(TrainerBy_fold);

typedef struct TrainerBy_apply {
    MANY(TrainerBy_fold) byfold;
} TrainerBy_apply;

MAKEMANY(TrainerBy_apply);

typedef struct TrainerBy_wsize {
    MANY(TrainerBy_apply) byapply;
} TrainerBy_wsize;

MAKEMANY(TrainerBy_wsize);

typedef struct TrainerBy {
    struct {
        const size_t wsize;
        const size_t apply;
        const size_t fold;
        const size_t thchooser;
    } n;
    MANY(TrainerBy_wsize) bywsize;
} TrainerBy;

typedef struct __Trainer {
    RTestBed2 tb2;
    MANY(Performance) thchoosers;

    TrainerBy by;
} __Trainer;

typedef struct __Trainer* RTrainer;

#define FORBY(BY, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < BY.n.NAME; idx ## NAME++)

#define RESULT_IDX(BY, WSIZE, APPLY, FOLD, TRY, SPLIT, THCHOOSER) BY.bywsize._[WSIZE].byapply._[APPLY].byfold._[FOLD].bytry._[TRY].bysplits._[SPLIT].bythchooser._[THCHOOSER]

RTrainer trainer_run(RTestBed2, MANY(Performance), char rootdir[PATH_MAX - 100]);

void trainer_free(RTrainer results);

void trainer_io(IOReadWrite rw, char dirname[200], RTestBed2, RTrainer* results);

#endif