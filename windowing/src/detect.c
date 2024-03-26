#include "detect.h"

#include "windowing.h"
#include "windowmany.h"
#include "window0many.h"

#include <assert.h>
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

void detect_update(WApply const * const apply, RSource source, const double th, Detection* detection) {
    const DetectionValue tp = apply->logit >= th;
    
    detection->th = th;
    detection->windows[source->wclass.mc][tp]++;
    detection->sources[source->g2index][tp]++;
    
    for (size_t idxdnbad = 0; idxdnbad < N_WAPPLYDNBAD; idxdnbad++) {
        detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
    }
    detection->dn_count[source->wclass.mc] += apply->wcount;
    detection->dn_whitelistened_count[source->wclass.mc] += apply->whitelistened;
}

void detect_run(Detection* detection, RWindowMany windowmany, size_t const idxconfig, const double th, const double thzone[N_DETZONE]) {
    memset(detection, 0, sizeof(Detection));

    detection->windowmany = windowmany;

    detection->th = th;
    memcpy(detection->thzone, thzone, sizeof(double) * N_DETZONE);

    for (size_t w = 0; w < windowmany->number; w++) {
        RWindow window = windowmany->_[w];
        WApply* apply = &windowmany->_[w]->applies._[idxconfig];
        RSource source = window->windowing->source;
        WClass wc = window->windowing->source->wclass;

        const DetectionValue tp = apply->logit >= detection->th;

        assert(wc.mc < N_DGACLASSES);
    
        detection->windows[wc.mc][tp]++;
        detection->sources[source->g2index][tp]++;
    
        for (size_t idxdnbad = 0; idxdnbad < N_WAPPLYDNBAD; idxdnbad++) {
            detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
        }
        detection->dn_count[source->wclass.mc] += apply->wcount;
        detection->dn_whitelistened_count[source->wclass.mc] += apply->whitelistened;

        {
            int z = 0;
            for (z = 0; z < N_DETZONE - 1; z++) {
                if (apply->logit >= detection->thzone[z] && apply->logit < detection->thzone[z + 1]) {
                    break;
                }
            }
            assert(z >= 0 && z <= N_DETZONE - 1);
            detection->zone[z][source->wclass.mc]++;
        }
    }
}

double detect_performance(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    return performance->func(detection, performance);
}

double detect_true0ratio(Detection detection[N_DGACLASSES], DGAClass mc) {
    return ((double) detection->windows[mc][1]) / (detection->windows[mc][0] + detection->windows[mc][1]);
}

int detect_performance_compare(Performance* performance, double new, double old) {
    double diff = new - old;
    
    if (!performance->greater_is_better)
        diff *= -1;

    return diff > 0;
}