
#ifndef __DETECT_H__
#define __DETECT_H__

#include "dataset.h"
#include "sources.h"
#include "windows.h"

typedef struct CM {
    int32_t falses;
    int32_t trues;
} CM;

typedef struct CMs {
    int32_t number; // number of trues/falses windows for each source
    CM* _; // number of trues/falses windows for each source
} CMs;

typedef struct CM2 {
    CM windows;
    CMs sources;
} CM2;

MAKEDGA(CM2);
MAKEMANY(CM2);
MAKEDGAMANY(CM2);

typedef struct Detection {
    double th;
    DGA(CM2) cm2dga;
} Detection;

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

typedef double (*PerformanceFunctionPtr)(Detection*, struct Performance*);

typedef struct Performance {
    char name[20];

    enum PerfomanceDGAHandling dgadet;

    PerformanceFunctionPtr func;
    
    int greater_is_better;
} Performance;

MAKEMANY(Performance);



void detect_reset(Detection*);
void detect_init(Detection*);
void detect_free(Detection*);
void detect_copy(Detection* src, Detection* dst);

void detect_run(DGAMANY(RWindow), double, Detection*);

double detect_performance(Detection*, Performance*);

int detect_performance_compare(Performance*, double, double);

#endif