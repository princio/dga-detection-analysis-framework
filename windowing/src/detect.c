#include "detect.h"

#include "stratosphere_window.h"
#include "windowing.h"
#include "windowmany.h"
#include "window0many.h"

#include <assert.h>
#include <float.h>
#include <math.h>
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
    detection->dn_whitelistened_unique_count[source->wclass.mc] += apply->whitelistened_unique;
    detection->dn_whitelistened_total_count[source->wclass.mc] += apply->whitelistened_total;
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
        detection->dn_whitelistened_total_count[source->wclass.mc] += apply->whitelistened_total;
        detection->dn_whitelistened_unique_count[source->wclass.mc] += apply->whitelistened_unique;

        {
            int z;
            DetectionZone *zone;
            zone = &detection->zone.dn;
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
            memcpy(detection->zone.llr.th, thzone, N_DETZONE * sizeof(double));
            memcpy(detection->zone.days[source->day - 1].th, thzone, N_DETZONE * sizeof(double));
            int nobreak = 1;
            for (z = 0; z < N_DETZONE - 1; z++) {
                if (apply->logit >= thzone[z] && apply->logit < thzone[z + 1]) {
                    nobreak = 0;
                    break;
                }
            }
            assert(z >= 0 && z < N_DETZONE - 1);
            detection->zone.llr.zone[z][source->wclass.mc]++;
            detection->zone.days[source->day - 1].zone[z][wc.mc]++;

            if (z == (N_DETZONE - 2) && source->wclass.mc == 0) {
                printf(
                    "\nSELECT  * "
                    "FROM "
                    "(SELECT *  FROM MESSAGE_%d WHERE FN_REQ >= %d AND FN_REQ < %d) M "
                    "JOIN DN ON M.DN_ID = DN.ID "
                    "JOIN DN_NN ON DN.ID = DN_NN.DN_ID "
                    "AND DN_NN.NN_ID = %d\n",
                    window->windowing->source->id,
                    window->fn_req_min, window->fn_req_max,
                    configsuite.configs._[idxconfig].nn
                );
                double llrs[4];
                stratosphere_window_llr(window, llrs);

                double llrdb = llrs[configsuite.configs._[0].nn];

                double llr = window->applies._[0].logit;

                if (fabs(llrdb - llr) > 0.001) {
                    printf("%f - %f = %f\n", llrdb, llr, llrdb - llr);
                    printf("\nwindow: %s -> %d to %d\n", window->windowing->source->name, window->fn_req_min, window->fn_req_max);
                }
            }
        }
    }
}

void detect_alarms(DetectionZone* detectionzone, DetectionValue* false_alarms, DetectionValue* true_alarms) {
    *false_alarms = 0;
    *true_alarms = 0;
    
    for (size_t z = (N_DETZONE - 1) / 2; z < N_DETZONE - 1; z++) {
        DGAFOR(mc) {
            if (mc == 0) {
                *false_alarms += detectionzone->zone[z][mc];
            } else {
                *true_alarms += detectionzone->zone[z][mc];
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