#ifndef __WINDOWING_WINDOWING_H__
#define __WINDOWING_WINDOWING_H__

#include "common.h"

#include "gatherer2.h"
#include "source.h"
#include "window0many.h"

typedef struct __Windowing {
    G2Index g2index;

    RSource source;
    size_t wsize;
    RWindow0Many window0many;
} __Windowing;

MAKEMANY(__Windowing);

IndexMC windowing_count();

void windowing_build(RWindowing, RWindow0Many, size_t, RSource);

void windowing_apply(WSize wsize);

#endif