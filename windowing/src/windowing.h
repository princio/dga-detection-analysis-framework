#ifndef __WINDOWING_WINDOWING_H__
#define __WINDOWING_WINDOWING_H__

#include "common.h"

#include "gatherer2.h"
#include "source.h"
#include "windowmany.h"

typedef struct __Windowing {
    G2Index g2index;

    RSource source;
    size_t wsize;
    RWindowMany windowmany;
} __Windowing;

IndexMC windowing_many_count(MANY(RWindowing) windowingmany);

void windowing_build(RWindowing, RWindow0Many, size_t, RSource);

void windowing_apply(WSize wsize);

#endif