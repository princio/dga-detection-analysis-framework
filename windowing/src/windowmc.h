
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

void windowmc_clone(RWindowMC, RWindowMC);

void windowmc_init(RWindowMC windowmc);

void windowmc_buildby_windowmany(RWindowMC rwindowmc, RWindowMany windowmany);
void windowmc_buildby_windowing_many(RWindowMC rwindowmc, MANY(RWindowing) windowingmany);
void windowmc_buildby_size(RWindowMC rwindowmc, IndexMC size);

#endif