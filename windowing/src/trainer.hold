
#ifndef __TRAINER_H__
#define __TRAINER_H__

#include "common.h"

#include "detect.h"
#include "windowmany.h"

#include <linux/limits.h>

typedef struct TrainerBy_config {
    Performance* threshold_chooser;
    float fpr_grt09;
    Detection best_train;
    Detection best_test;
} TrainerBy_config;

MAKEMANY(TrainerBy_config);

typedef struct TrainerBy_thchooser {
    MANY(TrainerBy_config) byconfig;
} TrainerBy_thchooser;
MAKEMANY(TrainerBy_thchooser);

typedef struct TrainerBy_split {
    MANY(TrainerBy_thchooser) bythchooser;
} TrainerBy_split;

MAKEMANY(TrainerBy_split);

typedef struct TrainerBy_try {
    int isok;
    MANY(TrainerBy_split) bysplit;
} TrainerBy_try;

MAKEMANY(TrainerBy_try);

typedef struct TrainerBy_fold {
    MANY(TrainerBy_try) bytry;
} TrainerBy_fold;

MAKEMANY(TrainerBy_fold);

typedef struct TrainerBy {
    struct {
        size_t fold;
        size_t try;
        size_t thchooser;
        size_t config;
    } n;
    MANY(TrainerBy_fold) byfold;
} TrainerBy;

typedef struct __Trainer {
    char rootdir[DIR_MAX];
    MANY(Performance) thchoosers;
    TrainerBy by;
} __Trainer;

#define RESULT_IDX(BY, CONFIG, FOLD, TRY, SPLIT, THCHOOSER) BY.byconfig._[CONFIG].byfold._[FOLD].bytry._[TRY].bysplits._[SPLIT].bythchooser._[THCHOOSER]

RTrainer trainer_run(RTB2D, MANY(Performance), char rootdir[DIR_MAX]);
RTrainer trainer_run2(RTB2D tb2d, MANY(Performance) thchoosers, char rootdir[DIR_MAX]);

void trainer_free(RTrainer results);

void trainer_io(IOReadWrite rw, char dirname[200], RTB2D, RTrainer* results);

#endif