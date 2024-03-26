
#ifndef __WINDOWING_WINDOW0MANY_H__
#define __WINDOWING_WINDOW0MANY_H__

#include "common.h"

#include "gatherer2.h"
#include "wapply.h"

typedef struct __Window {
    G2Index manyindex;

    size_t index;
    
    RWindowing windowing;
    
    float duration;

    uint32_t fn_req_min;
    uint32_t fn_req_max;

    size_t n_message;
    MANY(WApply) applies;
} __Window;

typedef struct __Window0Many {
    G2Index g2index;

    RWindowing windowing;
    
    size_t element_size;
    size_t size;
    size_t number;
    __Window* _;
} __Window0Many;

void window0many_buildby_size(RWindow0Many, const size_t);

RWindowMany window0many_create(size_t);

#endif