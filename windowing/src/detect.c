#include "detect.h"

#include "windowing.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

void _detect_quickSort(double arr[], int low, int high);

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

void detect_run(WApply const * const apply, RSource source, Detection* detection) {
    const DetectionValue tp = apply->logit >= detection->th;
    detection->windows[source->wclass.mc][tp]++;
    detection->sources[source->wclass.mc][source->index.multi][tp]++;
    for (size_t idxdnbad = 0; idxdnbad < N_WAPPLYDNBAD; idxdnbad++) {
        detection->alarms[source->wclass.mc][idxdnbad] += apply->dn_bad[idxdnbad];
    }
    detection->dn_count[source->wclass.mc] += apply->wcount;
    detection->dn_whitelistened_count[source->wclass.mc] += apply->whitelistened;
}

void detect_fast(MANY(RWindow) windows, WClass wc, size_t idxconfig, MANY(double) ths, MANY(Detection) detection) {
    double logits[windows.number];
    for (size_t i = 0; i < windows.number; i++) {
        logits[i] = windows._[i]->applies._[idxconfig].logit;
    }
    
    // quickSort(logits, 0, windows.number - 1);

    for (size_t idxth = 0; idxth < ths.number; idxth++) {

    }
}

double detect_performance(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    return performance->func(detection, performance);
}

int detect_performance_compare(Performance* performance, double new, double old) {
    double diff = new - old;
    
    if (!performance->greater_is_better)
        diff *= -1;

    return diff > 0;
}

void _detect_swap(double* a, double* b) { 
    double temp = *a; 
    *a = *b; 
    *b = temp; 
} 
  
int _detect_partition(double arr[], int low, int high) { 
    double pivot = arr[low]; 
    int i = low; 
    int j = high; 
  
    while (i < j) { 
        while (arr[i] <= pivot && i <= high - 1) { 
            i++; 
        } 
        while (arr[j] > pivot && j >= low + 1) { 
            j--; 
        } 
        if (i < j) { 
            _detect_swap(&arr[i], &arr[j]); 
        } 
    } 
    _detect_swap(&arr[low], &arr[j]); 
    return j; 
} 
  
void _detect_quickSort(double arr[], int low, int high) { 
    if (low < high) { 
        int partitionIndex = _detect_partition(arr, low, high); 
        _detect_quickSort(arr, low, partitionIndex - 1); 
        _detect_quickSort(arr, partitionIndex + 1, high); 
    } 
} 