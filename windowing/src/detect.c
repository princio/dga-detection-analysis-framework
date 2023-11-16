#include "detect.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

void detect_reset(Detection* det) {
    // det->th = -1 * DBL_MIN;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        memset(det->sources, 0, MAX_SOURCEs * sizeof(CM));
        memset(&det->windows, 0, sizeof(CM));
    }
}

void detect_copy(TCPC(Detection) src, Detection* dst) {
    memcpy(dst, src, sizeof(Detection));
}

void detect_run(MANY(RWindow) ds, const double th, Detection* const detection) {
    detect_reset(detection);

    for (int32_t i = 0; i < ds.number; i++) {
        RWindow window = ds._[i];

        const int prediction = window->logit >= th;
        const int infected = window->dgaclass > 0;

        if (prediction == infected) {
            detection->windows.trues++;
            detection->sources[window->source_index.all].trues++;
        } else {
            detection->windows.falses++;
            detection->sources[window->source_index.all].falses++;
        }
    }
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