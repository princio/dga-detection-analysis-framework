#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "parameters.h"
#include "sources.h"
#include "window0s.h"

typedef struct __Windowing {
    size_t index;
    RSource source;
    WSize wsize;
    MANY(RWindow0) windows;
} __Windowing;

RWindowing windowings_alloc();
RWindowing windowings_create(WSize wsize, RSource source);

#endif