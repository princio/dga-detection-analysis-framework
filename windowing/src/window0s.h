
#ifndef __WINDOWS0_H__
#define __WINDOWS0_H__

#include "common.h"

#include "wapply.h"

typedef struct __Window0 {
    size_t index;
    RWindowing windowing;
    
    float duration;

    uint32_t fn_req_min;
    uint32_t fn_req_max;
    
    MANY(WApply) applies;
} __Window0;

void window0s_shuffle(MANY(RWindow0) rwindows);

MANY(RWindow0) window0s_alloc(size_t window0s_number);

#endif