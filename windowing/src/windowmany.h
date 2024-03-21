
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

IndexMC windowmany_count(RWindowMany);

void windowmany_shuffle(RWindowMany rwindows);

RWindowMany windowmany_alloc();

RWindowMany windowmany_alloc_size(size_t windows_num);

void window0many_io(IOReadWrite rw, FILE* file, RWindowMany windowmany);

#endif