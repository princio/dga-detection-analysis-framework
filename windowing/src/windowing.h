#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "sources.h"
#include "windows.h"

typedef struct __Windowing {
    size_t index;
    RSource source;
    WSize wsize;
    MANY(RWindow) windows;
} __Windowing;

RWindowing windowings_alloc();
RWindowing windowings_create(WSize wsize, RSource source);

#endif