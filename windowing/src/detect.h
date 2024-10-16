
#ifndef __DETECT_H__
#define __DETECT_H__

#include "common.h"

#include "wapply.h"

#define N_PERFORMANCE_DGAHANDLINGs 2

enum PerformanceRange {
    PERFORMANCE_RANGE_01,
    PERFORMANCE_RANGE_INF,
    PERFORMANCE_RANGE_INT_POS,
};

enum PerfomanceDGAHandling {
    PERFORMANCE_DGAHANDLING_IGNORE_1,
    PERFORMANCE_DGAHANDLING_MERGE_1
};

struct Performance;

typedef double (*PerformanceFunctionPtr)(Detection[N_DGACLASSES], TCPC(struct Performance));

typedef struct Performance {
    char name[20];
    enum PerfomanceDGAHandling dgadet;
    PerformanceFunctionPtr func;
    int greater_is_better;
} Performance;

MAKEMANY(Performance);

void detect_reset(Detection*);
void detect_copy(TCPC(Detection) src, Detection* dst);
void detect_update(WApply const * const apply, RSource source, const double th, Detection* detection);

void detect_run(Detection*, RWindowMany, size_t const, const double, const double[N_DETBOUND]);

void detect_alarms(DetectionZone*, DetectionValue* false_alarms, DetectionValue* true_alarms);

double detect_performance(Detection[N_DGACLASSES], TCPC(Performance));
int detect_performance_compare(Performance* performance, double new, double old);
double detect_true0ratio(Detection[N_DGACLASSES], DGAClass);


Detection* detection_alloc();

typedef struct StatDetectionBound {
    double min;
    double max;
    double avg;
    double avg_denominator;
    int divided;
} StatDetectionBound;

typedef struct StatDetectionValue {
    uint32_t min;
    uint32_t max;
    double avg;
    uint32_t avg_denominator;
    int divided;
} StatDetectionValue;

typedef struct StatDetectionZone {
    uint32_t detections_involved[N_DGACLASSES]; 
    StatDetectionValue _[N_DETZONE][N_DGACLASSES]; 
} StatDetectionZone;

typedef struct StatDetectionCount {
    StatDetectionZone all;
    StatDetectionZone days[7];
} StatDetectionCount;

typedef struct StatDetectionCountZone {
    StatDetectionBound zones_boundaries[N_DETBOUND];
    StatDetectionCount dn;
    StatDetectionCount llr;
} StatDetectionCountZone;

MAKEMANY(StatDetectionCountZone);

void detection_stat(MANY(StatDetectionCountZone) avg[N_PARAMETERS]);

#endif