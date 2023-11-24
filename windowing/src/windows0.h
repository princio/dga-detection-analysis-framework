
#ifndef __WINDOWS0_H__
#define __WINDOWS0_H__

#include "common.h"

#include "wapply.h"

typedef struct __Window0 {
    RWindowing windowing;
    int64_t fn_req_min;
    int64_t fn_req_max;
    double duration;
    MANY(WApply) applies;
} __Window0;

MAKETETRA(MANY(RWindow0));

typedef struct Dataset0 {
    WSize wsize;
    MANY(RWindow0) windows[N_DGACLASSES];
} Dataset0;

MAKEMANY(Dataset0);

MANY(RWindow0) rwindows0_from(MANY(RWindow0) rwindows_src);

void windows0_shuffle(MANY(RWindow0) rwindows);

MANY(RWindow0) windows0_alloc(const int32_t num);

void windows0_free();

void dataset0_free(Dataset0* dataset);

#endif