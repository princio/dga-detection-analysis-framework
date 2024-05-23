#include "detect.h"

#include "gatherer2.h"
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
    
    for (size_t idxdnbad = 0; idxdnbad < N_DETBOUND; idxdnbad++) {
        detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
    }
    detection->dn_count[source->wclass.mc] += apply->wcount;
    detection->dn_whitelistened_unique_count[source->wclass.mc] += apply->whitelistened_unique;
    detection->dn_whitelistened_total_count[source->wclass.mc] += apply->whitelistened_total;
}

void detect_run(Detection* detection, RWindowMany windowmany, size_t const idxconfig, const double th, const double thzone[N_DETBOUND]) {
    memset(detection, 0, sizeof(Detection));

    detection->windowmany = windowmany;

    detection->th = th;
    detection->idxconfig = idxconfig;

    memcpy(detection->zone.bounds, thzone, N_DETBOUND * sizeof(double));

    for (size_t w = 0; w < windowmany->number; w++) {
        RWindow window = windowmany->_[w];
        WApply* apply = &windowmany->_[w]->applies._[idxconfig];
        RSource source = window->windowing->source;
        WClass wc = window->windowing->source->wclass;

        assert(source->day >= -1 && source->day < 7);

        const DetectionValue tp = apply->logit >= detection->th;

        assert(wc.mc < N_DGACLASSES);
    
        detection->windows[wc.mc][tp]++;
        detection->sources[source->g2index][tp]++;
    
        for (size_t idxdnbad = 0; idxdnbad < N_DETBOUND; idxdnbad++) {
            detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
        }

        detection->dn_count[source->wclass.mc] += apply->wcount;
        detection->dn_whitelistened_total_count[source->wclass.mc] += apply->whitelistened_total;
        detection->dn_whitelistened_unique_count[source->wclass.mc] += apply->whitelistened_unique;

        for (size_t z = 0; z < N_DETZONE; z++) {
            detection->zone.dn.all._[z][wc.mc] += apply->dn_bad[z];
            if (source->day >= 0 && source->day < 7) {
                detection->zone.dn.days[source->day]._[z][wc.mc] += apply->dn_bad[z];
            }
            if (apply->logit >= thzone[z] && apply->logit < thzone[z + 1]) {
                detection->zone.llr.all._[z][wc.mc]++;
                if (source->day >= 0 && source->day < 7) {
                    detection->zone.llr.days[source->day]._[z][wc.mc]++;
                }
            }
        }
    }
}

void detect_alarms(DetectionZone* detectionzone, DetectionValue* false_alarms, DetectionValue* true_alarms) {
    *false_alarms = 0;
    *true_alarms = 0;
    
    for (size_t z = (N_DETZONE) / 2; z < N_DETZONE; z++) {
        DGAFOR(mc) {
            if (mc == 0) {
                *false_alarms += detectionzone->_[z][mc];
            } else {
                *true_alarms += detectionzone->_[z][mc];
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

///////////////////////////////////////////////////////////////////////

void _detect_free(void*);
void _detect_io(IOReadWrite, FILE*, void**);
void _detect_print(void*);
void _detect_hash(void*, SHA256_CTX*);

G2Config g2_config_detection = {
    .element_size = sizeof(Detection),
    .size = 0,
    .freefn = _detect_free,
    .iofn = _detect_io,
    .printfn = _detect_print,
    .hashfn = _detect_hash,
    .id = G2_DETECTION
};

Detection* detection_alloc() {
    return (Detection*) g2_insert_alloc_item(G2_DETECTION);
}

void _detect_free(void* item) {
}

void _detect_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    g2_io_call(G2_WMANY, rw);

    Detection** detection = (Detection**) item;

    g2_io_index(file, rw, G2_WMANY, (void**) &(*detection)->windowmany);

    FRWSIZE(((*detection)->th), sizeof(Detection) - sizeof(G2Index) - sizeof(size_t) - sizeof(RWindowMC));
}

void _detect_print(void* item) {
    Detection* detection = (Detection*) item;

}

void _detect_hash(void* item, SHA256_CTX* sha) {
    Detection* detection = (Detection*) item;

    SHA256_Update(sha, &detection->th, sizeof(Detection) - sizeof(G2Index) - sizeof(size_t)  - sizeof(RWindowMC));
}

void detection_stat(MANY(StatDetectionCountZone) avg[N_PARAMETERS]) {
    __MANY many = g2_array(G2_DETECTION);

    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
        MANY_INIT(avg[pp], configsuite.realm[pp].number, StatDetectionCountZone);
        for (size_t p = 0; p < configsuite.realm[pp].number; p++) {
            for (size_t b = 0; b < N_DETBOUND; b++) {
                avg[pp]._[p].zones_boundaries[b].avg = 0;
                avg[pp]._[p].zones_boundaries[b].avg_denominator = 0;
                avg[pp]._[p].zones_boundaries[b].divided = 0;
                avg[pp]._[p].zones_boundaries[b].min = DBL_MAX;
                avg[pp]._[p].zones_boundaries[b].max = - DBL_MAX;
            }
            for (size_t z = 0; z < N_DETZONE; z++) {
                DGAFOR(cl) {
                    avg[pp]._[p].dn.all._[z][cl].avg = 0;
                    avg[pp]._[p].dn.all._[z][cl].min = UINT32_MAX;
                    avg[pp]._[p].dn.all._[z][cl].max = 0;

                    avg[pp]._[p].llr.all._[z][cl].avg = 0;
                    avg[pp]._[p].llr.all._[z][cl].min = UINT32_MAX;
                    avg[pp]._[p].llr.all._[z][cl].max = 0;

                    for (size_t day = 0; day < 7; day++) {
                        avg[pp]._[p].dn.days[day]._[z][cl].avg = 0;
                        avg[pp]._[p].dn.days[day]._[z][cl].min = UINT32_MAX;
                        avg[pp]._[p].dn.days[day]._[z][cl].max = 0;

                        avg[pp]._[p].llr.days[day]._[z][cl].avg = 0;
                        avg[pp]._[p].llr.days[day]._[z][cl].min = UINT32_MAX;
                        avg[pp]._[p].llr.days[day]._[z][cl].max = 0;
                    }
                }
            }
        }
    }

    for (size_t d = 0; d < many.number; d++) {
        Detection* detection = (Detection*) many._[d];
        Config* config = &configsuite.configs._[detection->idxconfig];

        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            StatDetectionCountZone* sdcz = &avg[pp]._[config->parameters[pp]->index];

            for (size_t b = 0; b < N_DETBOUND; b++) {
                sdcz->zones_boundaries[b].avg += detection->zone.bounds[b];
                sdcz->zones_boundaries[b].min = sdcz->zones_boundaries[b].min > detection->zone.bounds[b] ? detection->zone.bounds[b] : sdcz->zones_boundaries[b].min;
                sdcz->zones_boundaries[b].max = sdcz->zones_boundaries[b].max < detection->zone.bounds[b] ? detection->zone.bounds[b] : sdcz->zones_boundaries[b].max;
                sdcz->zones_boundaries[b].avg_denominator++;
            }
            for (size_t z = 0; z < N_DETZONE; z++) {
                DGAFOR(cl) {

#define BIBO(A, B) \
    if (detection->zone.B._[z][cl]) {\
        (A)->B._[z][cl].avg += detection->zone.B._[z][cl]; \
        (A)->B._[z][cl].avg_denominator++; \
        (A)->B.detections_involved[cl]++; \
        if ((A)->B._[z][cl].min > detection->zone.B._[z][cl]) { \
            (A)->B._[z][cl].min = detection->zone.B._[z][cl]; \
        } \
        if ((A)->B._[z][cl].max < detection->zone.B._[z][cl]) { \
            (A)->B._[z][cl].max = detection->zone.B._[z][cl]; \
        }\
    }
                    BIBO(sdcz, dn.all);
                    BIBO(sdcz, llr.all);

                    for (size_t day = 0; day < 7; day++) {
                        BIBO(sdcz, dn.days[day]);
                        BIBO(sdcz, llr.days[day]);
                    }
#undef BIBO
                }
            }
        }
    }

    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
        for (size_t p = 0; p < configsuite.realm[pp].number; p++) {
            StatDetectionCountZone* sdcz = &avg[pp]._[p];
            for (size_t b = 0; b < N_DETBOUND; b++) {
                sdcz->zones_boundaries[b].avg /= sdcz->zones_boundaries[b].avg_denominator;
            }
            for (size_t z = 0; z < N_DETZONE; z++) {
                DGAFOR(cl) {

#define BIBOAVG(A, B) if ((A)->B._[z][cl].avg_denominator) {\
    if ((A)->B._[z][cl].divided) printf("already divided\n");\
        (A)->B._[z][cl].divided = 1;\
        (A)->B._[z][cl].avg /= (A)->B._[z][cl].avg_denominator;\
    }
                    BIBOAVG(sdcz, dn.all);
                    BIBOAVG(sdcz, llr.all);

                    for (size_t day = 0; day < 7; day++) {
                        BIBOAVG(sdcz, dn.days[day]);
                        BIBOAVG(sdcz, llr.days[day]);
                    }
#undef BIBOAVG
                }
            }
        }
    }
}