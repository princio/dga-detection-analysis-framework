
#ifndef __WINDOWING_WINDOWMC_H__
#define __WINDOWING_WINDOWMC_H__

#include "common.h"

#include "gatherer2.h"
#include "wapply.h"

typedef struct __WindowMC {
    G2Index g2index;

    MANY(MinMax) minmax;
    RWindowMany all;
    RWindowMany binary[2];
    RWindowMany multi[N_DGACLASSES];
} __WindowMC;

void windowmc_shuffle(RWindowMC rwindows);

IndexMC windowmc_count(RWindowMC);

RWindowMC windowmc_clone(RWindowMC);

RWindowMC windowmc_alloc();

RWindowMC windowmc_alloc_by_windowmany(RWindowMany windowmany);

RWindowMC windowmc_alloc_by_windowingmany(MANY(RWindowing) windowingmany);

#endif