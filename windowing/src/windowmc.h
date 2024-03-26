
#ifndef __WINDOWING_WINDOWMC_H__
#define __WINDOWING_WINDOWMC_H__

#include "common.h"

#include "gatherer2.h"
#include "wapply.h"

typedef struct __WindowMC {
    G2Index g2index;

    RWindowMany all;
    RWindowMany binary[2];
    RWindowMany multi[N_DGACLASSES];
} __WindowMC;

void windowmc_shuffle(RWindowMC);

IndexMC windowmc_count(RWindowMC);

void windowmc_clone(RWindowMC, RWindowMC);

void windowmc_init(RWindowMC);

void windowmc_buildby_windowmany(RWindowMC, RWindowMany);
void windowmc_buildby_windowing(RWindowMC windowmc);
void windowmc_buildby_size(RWindowMC, IndexMC);

void windowmc_hash_update(SHA256_CTX*, RWindowMC);

#endif