
#ifndef __DETECT_H__
#define __DETECT_H__

#include "windows.h"

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
void detect_run(WApply const * const apply, RSource source, Detection* detection);
double detect_performance(Detection[N_DGACLASSES], TCPC(Performance));
int detect_performance_compare(Performance* performance, double new, double old);

#endif