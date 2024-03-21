#ifndef __WINDOWING_WINDOWING_H__
#define __WINDOWING_WINDOWING_H__

#include "gatherer2.h"
#include "source.h"
#include "windowmany.h"

typedef struct __Windowing {
    G2Index g2index;

    RSource source;
    size_t wsize;
    RWindowMany windowmany;
} __Windowing;

IndexMC windowingmany_count(MANY(RWindowing) windowingmany);

RWindowing windowing_alloc();

RWindowing windowing_create(size_t wsize, RSource source);

void windowing_apply(ConfigSuite suite);

#endif