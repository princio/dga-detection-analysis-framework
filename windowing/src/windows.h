
#ifndef __WINDOWS_H__
#define __WINDOWS_H__

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

void windows_shuffle(MANY(RWindow) rwindows);

MANY(RWindow) windows_alloc(size_t windows_number);

#endif