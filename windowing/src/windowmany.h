
#ifndef __WINDOWING_WINDOWMANY_H__
#define __WINDOWING_WINDOWMANY_H__

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

    MANY(WApply) applies;
} __Window;

MAKEMANY(__Window);

typedef struct __WindowMany {
    G2Index g2index;

    size_t n_apply;
    RWindowing windowing;
    
    MANY(MinMax) minmax;

    size_t element_size;
    size_t size;
    size_t number;
    RWindow* _;
} __WindowMany;

typedef struct __Window0Many {
    MANY(__Window) __window0many;
    __WindowMany __windowmany;
} __Window0Many;

IndexMC windowmany_count(RWindowMany);

void windowmany_shuffle(RWindowMany);

void windowmany_buildby_size(RWindowMany, size_t);

void window0many_buildby_size(RWindow0Many, const size_t);

RWindowMany windowmany_create(size_t);

RWindowMany window0many_create(size_t);

#endif