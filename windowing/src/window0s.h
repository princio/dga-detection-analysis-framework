
#ifndef __WINDOWS0_H__
#define __WINDOWS0_H__

#include "common.h"

#include "wapply.h"

typedef struct __Window0 {
    size_t index;
    RWindowing windowing;
    
    double duration;

    size_t fn_req_min;
    size_t fn_req_max;
    size_t applies_number;
    
    MANY(WApply) applies;
} __Window0;

MAKETETRA(MANY(RWindow0));

MANY(double) window0s_ths(const MANY(RWindow0) windows[N_DGACLASSES], const size_t pset_index);

MANY(RWindow0) rwindow0s_from(MANY(RWindow0) rwindows_src);

void window0s_shuffle(MANY(RWindow0) rwindows);

MANY(RWindow0) window0s_alloc(const int32_t num);

void window0s_free();

#endif