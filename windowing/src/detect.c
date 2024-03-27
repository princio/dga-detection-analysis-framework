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
    
    for (size_t idxdnbad = 0; idxdnbad < N_DETZONE; idxdnbad++) {
        detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
    }
    detection->dn_count[source->wclass.mc] += apply->wcount;
    detection->dn_whitelistened_count[source->wclass.mc] += apply->whitelistened;
}

void detect_run(Detection* detection, RWindowMany windowmany, size_t const idxconfig, const double th, const double thzone[N_DETZONE]) {
    memset(detection, 0, sizeof(Detection));

    detection->th = th;

    for (size_t w = 0; w < windowmany->number; w++) {
        RWindow window = windowmany->_[w];
        WApply* apply = &windowmany->_[w]->applies._[idxconfig];
        RSource source = window->windowing->source;
        WClass wc = window->windowing->source->wclass;

        const DetectionValue tp = apply->logit >= detection->th;

        assert(wc.mc < N_DGACLASSES);
    
        detection->windows[wc.mc][tp]++;
        detection->sources[source->g2index][tp]++;
    
        for (size_t idxdnbad = 0; idxdnbad < N_DETZONE; idxdnbad++) {
            detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
        }
        detection->dn_count[source->wclass.mc] += apply->wcount;
        detection->dn_whitelistened_count[source->wclass.mc] += apply->whitelistened;

        {
            int z;
            DetectionZone *zone;
            zone = &detection->zone[0];
            memcpy(zone->th, WApplyDNBad_Values, N_DETZONE * sizeof(double));
            int a = 0;
            for (z = 0; z < N_DETZONE; z++) {
                zone->zone[z][source->wclass.mc] += apply->dn_bad[z];
                a += apply->dn_bad[z];
            }
            assert(a);
        }
        {
            int z;
            DetectionZone *zone;
            zone = &detection->zone[1];
            memcpy(zone->th, thzone, N_DETZONE * sizeof(double));
            for (z = 0; z < N_DETZONE - 1; z++) {
                if (apply->logit >= zone->th[z] && apply->logit < zone->th[z + 1]) {
                    break;
                }
            }
            assert(z >= 0 && z <= N_DETZONE - 1);
            zone->zone[z][source->wclass.mc]++;
        }
    }
}

void detect_alarms(Detection* detection, int a, DetectionValue* false_alarms, DetectionValue* true_alarms) {
    *false_alarms = 0;
    *true_alarms = 0;
    
    for (size_t z = (N_DETZONE - 1) / 2; z < N_DETZONE - 1; z++) {
        DGAFOR(mc) {
            if (mc == 0) {
                *false_alarms += detection->zone[a].zone[z][mc];
            } else {
                *true_alarms += detection->zone[a].zone[z][mc];
            }
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