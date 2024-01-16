#include "detect.h"

#include "windowing.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

void detect_reset(Detection* det) {
    // det->th = -1 * DBL_MIN;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        memset(det->sources, 0, 100 * sizeof(uint16_t));
        memset(&det->windows, 0, sizeof(uint16_t));
    }
}

void detect_copy(TCPC(Detection) src, Detection* dst) {
    memcpy(dst, src, sizeof(Detection));
}

MANY(double) detect_many_performance(Detection detection[N_DGACLASSES], MANY(Performance) many_performance) {
    MANY(double) scores;
    MANY_INIT(scores, many_performance.number, double);
    for (size_t i = 0; i < many_performance.number; i++) {
        scores._[i] = many_performance._[i].func(detection, &many_performance._[i]);
    }
    return scores;
}

double detect_performance(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    return performance->func(detection, performance);
}

int detect_performance_compare(Performance* performance, double new, double old) {
    double diff = new - old;
    
    if (!performance->greater_is_better)
        diff *= -1;

    // printf("%30s\t%5.4f %s %5.4f? %s\n", performance->name, new, performance->greater_is_better ? ">" : "<", old, diff > 0 ? "better" : "not");

    return diff > 0;
}