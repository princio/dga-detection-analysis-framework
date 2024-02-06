
#ifndef __STAT_H__
#define __STAT_H__

#include "common.h"

#include "configsuite.h"
#include "trainer.h"
typedef struct StatMetricPN {
    size_t min;
    size_t max;
    double avg;
    double std;

    size_t min_over;
    size_t max_over;
    double avg_over;

    size_t count;
} StatMetricPN;

typedef struct StatMetricRatio {
    double min;
    double max;
    double avg;
    double std;
} StatMetricRatio;

typedef struct StatMetricAlarms {
    StatMetricPN unwindowed[N_WAPPLYDNBAD];
    StatMetricPN windowed;
    StatMetricPN sources;
} StatMetricAlarms;


typedef struct StatMetric {
    StatMetricRatio ratio;
    StatMetricAlarms alarms;
    size_t count;
} StatMetric;

typedef struct StatByPSetItemValue {
    char name[25];
    char svalue[100];
    StatMetric train[N_DGACLASSES];
    StatMetric test[N_DGACLASSES];
} StatByPSetItemValue;
MAKEMANY(StatByPSetItemValue);

typedef MANY(StatByPSetItemValue) StatBy_thchooser[N_PARAMETERS];

MAKEMANY(StatBy_thchooser);

typedef struct StatBy_fold {
    struct {
        size_t train;
        size_t test;
    } wcount[N_DGACLASSES];
    MANY(StatBy_thchooser) bythchooser;
} StatBy_fold;
MAKEMANY(StatBy_fold);

typedef struct StatBy {
    struct {
        size_t fold;
        size_t thchooser;
    } n;
    MANY(StatBy_fold) byfold;
} StatBy;

typedef struct Stat {
    RTrainer trainer;
    StatBy by;
} Stat;

#define STAT_IDX(STAT, FOLD, THCHOOSER, PP, PARAMETER) STAT.by.byfold._[FOLD].bythchooser._[THCHOOSER][PP]._[PARAMETER]

Stat stat_run(RTrainer, ParameterRealmEnabled parameterrealmenabled, char* csvpath);

void stat_free(Stat results);

#endif