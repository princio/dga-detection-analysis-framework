
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

void window0s_shuffle(MANY(RWindow0) rwindows);

MANY(RWindow0) window0s_alloc(size_t window0s_number);

#endif