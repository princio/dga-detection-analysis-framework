
#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include "common.h"

#include "parameters.h"
#include "window0s.h"

typedef struct __Window {
    RWindow0 w0;
    
    IDX pset_index;

    int32_t wcount;
    double  logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} __Window;

MAKEMANY(RWindow);

MANY(double) windows_ths(const MANY(RWindow0) windows[N_DGACLASSES], const size_t pset_index);

MANY(RWindow) rwindows_from(MANY(RWindow) rwindows_src);

void rwindows_shuffle(MANY(RWindow));

MANY(RWindow) windows_alloc(const int32_t num);

void windows_free();

void windows_calc(const DNSMessage message, TCPC(PSet) pset, RWindow window);

#endif