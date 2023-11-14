#include "detect.h"

#include <float.h>
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

Detection* detect_run(MANY(RWindow) ds, const double th) {
    Detection* detection = calloc(1, sizeof(Detection));

    detect_reset(detection);

    for (int32_t i = 0; i < ds.number; i++) {
        RWindow window = ds._[i];

        const int cl = window->dgaclass;
        const int prediction = window->logit >= th;
        const int infected = window->dgaclass > 0;

        if (prediction == infected) {
            detection->windows.trues++;
            detection->sources[window->index.multi[cl]].trues++;
        } else {
            detection->windows.falses++;
            detection->sources[window->index.multi[cl]].falses++;
        }
    }

    return detection;
}

double detect_performance(Detection* detection[N_DGACLASSES], TCPC(Performance) performance) {
    return performance->func(detection, performance);
}

int detect_performance_compare(Performance* performance, double a, double b) {
    double diff = a - b;
    
    if (performance->greater_is_better)
        diff *= -1;

    return diff > 0;
}