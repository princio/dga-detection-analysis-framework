
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
    StatMetric tr[N_DGACLASSES];
} StatByPSetItemValue;
MAKEMANY(StatByPSetItemValue);

typedef struct StatByThChooser {
    MANY(StatByPSetItemValue) byninf;
    MANY(StatByPSetItemValue) bypinf;
    MANY(StatByPSetItemValue) bynn;
    MANY(StatByPSetItemValue) bywl_rank;
    MANY(StatByPSetItemValue) bywl_value;
    MANY(StatByPSetItemValue) bywindowing;
    MANY(StatByPSetItemValue) bynx_epsilon_increment;
} StatByThChooser;
MAKEMANY(StatByThChooser);

typedef struct StatByWSize {
    MANY(StatByThChooser) bythchooser;
} StatByWSize;
MAKEMANY(StatByWSize);

typedef struct StatByFold {
    MANY(StatByWSize) bywsize;
} StatByFold;
MAKEMANY(StatByFold);

typedef struct StatBy {
    MANY(StatByFold) byfold;
} StatBy;

typedef struct Stat {
    RTrainer trainer;
    StatBy by;
} Stat;

#define STAT_IDX(STAT, FOLD, WSIZE, THCHOOSER, PSETNAME, PSETITEM) STAT.by.byfold._[FOLD].bywsize._[WSIZE].bythchooser._[THCHOOSER].by ## PSETNAME._[PSETITEM]

Stat stat_run(RTrainer);

void stat_free(Stat results);

void stat_io(IOReadWrite rw, char dirname[200], RTrainer);

#endif