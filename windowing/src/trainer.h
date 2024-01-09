
#ifndef __TRAINER_H__
#define __TRAINER_H__

#include "common.h"

#include "detect.h"
#include "windows.h"
#include "tb2d.h"

#include <linux/limits.h>

typedef struct TrainerBy_splits {
    Performance* threshold_chooser;
    Detection best_train;
    Detection best_test;
} TrainerBy_splits;

MAKEMANY(TrainerBy_splits);

typedef struct TrainerBy_thchooser {
    MANY(TrainerBy_splits) splits;
} TrainerBy_thchooser;
MAKEMANY(TrainerBy_thchooser);

typedef struct TrainerBy_try {
    MANY(TrainerBy_thchooser) bythchooser;
} TrainerBy_try;

MAKEMANY(TrainerBy_try);

typedef struct TrainerBy_fold {
    MANY(TrainerBy_try) bytry;
} TrainerBy_fold;

MAKEMANY(TrainerBy_fold);

typedef struct TrainerBy_config {
    MANY(TrainerBy_fold) byfold;
} TrainerBy_config;

MAKEMANY(TrainerBy_config);

typedef struct TrainerBy {
    struct {
        const size_t config;
        const size_t fold;
        const size_t try;
        const size_t thchooser;
    } n;
    MANY(TrainerBy_config) byconfig;
} TrainerBy;

typedef struct __Trainer {
    RTB2D tb2d;
    MANY(Performance) thchoosers;
    TrainerBy by;
} __Trainer;

typedef struct __Trainer* RTrainer;

#define RESULT_IDX(BY, CONFIG, FOLD, TRY, SPLIT, THCHOOSER) BY.byconfig._[CONFIG].byfold._[FOLD].bytry._[TRY].bysplits._[SPLIT].bythchooser._[THCHOOSER]

RTrainer trainer_run(RTB2D, MANY(Performance), char rootdir[DIR_MAX]);

void trainer_free(RTrainer results);

void trainer_io(IOReadWrite rw, char dirname[200], RTB2D, RTrainer* results);

#endif