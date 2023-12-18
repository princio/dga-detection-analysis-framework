
#ifndef __STAT_H__
#define __STAT_H__

#include "common.h"

#include "parameters.h"
#include "testbed2.h"
#include "trainer.h"

typedef struct StatMetric {
    double min;
    double max;
    double avg;
    double std;
    size_t count;
} StatMetric;

typedef struct StatByPSetItemValue {
    char name[25];
    char svalue[100];
    StatMetric train[N_DGACLASSES];
    StatMetric test[N_DGACLASSES];
} StatByPSetItemValue;
MAKEMANY(StatByPSetItemValue);

typedef struct StatBy_thchooser {
    MANY(StatByPSetItemValue) byninf;
    MANY(StatByPSetItemValue) bypinf;
    MANY(StatByPSetItemValue) bynn;
    MANY(StatByPSetItemValue) bywl_rank;
    MANY(StatByPSetItemValue) bywl_value;
    MANY(StatByPSetItemValue) bywindowing;
    MANY(StatByPSetItemValue) bynx_epsilon_increment;
} StatBy_thchooser;
MAKEMANY(StatBy_thchooser);

typedef struct StatBy_wsize {
    MANY(StatBy_thchooser) bythchooser;
} StatBy_wsize;
MAKEMANY(StatBy_wsize);

typedef struct StatBy_fold {
    MANY(StatBy_wsize) bywsize;
} StatBy_fold;
MAKEMANY(StatBy_fold);

typedef struct StatBy {
    struct {
        const size_t fold;
        const size_t wsize;
        const size_t thchooser;
    } n;
    MANY(StatBy_fold) byfold;
} StatBy;

typedef struct Stat {
    RTrainer trainer;
    StatBy by;
} Stat;

#define STAT_IDX(STAT, FOLD, WSIZE, THCHOOSER, PSETNAME, PSETITEM) STAT.by.byfold._[FOLD].bywsize._[WSIZE].bythchooser._[THCHOOSER].by ## PSETNAME._[PSETITEM]

Stat stat_run(RTrainer, PSetByField psetitems_toignore, char fpath[PATH_MAX]);

void stat_free(Stat results);

#endif