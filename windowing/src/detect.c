#include "detect.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

int32_t NSOURCES[10];

void detect_reset(Detection* det) {
    det->th = -1 * DBL_MIN;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        memset(det->cm2dga[cl].sources._, 0, det->cm2dga[cl].sources.number * sizeof(CM));
        memset(&det->cm2dga[cl].windows, 0, sizeof(CM));
    }
}


void detect_init(Detection* det, const Index nsources) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        INITMANY(det->cm2dga[cl].sources, nsources.multi[cl], CM);
    }

    detect_reset(det);
}

void detect_free(Detection* det) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        free(det->cm2dga[cl].sources._);
    }
}

void detect_copy(TCPC(Detection) src, Detection* dst) {
    memcpy(dst, src, sizeof(Detection));
}

void detect_run(const DGAMANY(RWindow) drw, const double th, Detection* det) {
    detect_reset(det);

    det->th = th;

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        CM2* cm = &det->cm2dga[cl];

        for (int32_t i = 0; i < drw[cl].number; i++) {
            Window* window = drw[cl]._[i];
            int prediction = window->logit >= th;
            int infected = cl > 0;

            if (prediction == infected) {
                cm->windows.trues++;
                cm->sources._[window->index.multi[cl]].trues++;
            } else {
                cm->windows.falses++;
                cm->sources._[window->index.multi[cl]].falses++;
            }
        }
    }
}

double detect_performance(TCPC(Detection) detection, Performance* performance) {
    return performance->func(detection, performance);
}

int detect_performance_compare(Performance* performance, double a, double b) {
    double diff = a - b;
    
    if (performance->greater_is_better)
        diff *= -1;

    return diff > 0;
}