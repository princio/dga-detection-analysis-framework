
#ifndef __WINDOWING_WINDOWMANY_H__
#define __WINDOWING_WINDOWMANY_H__

#include "common.h"

#include "gatherer2.h"
#include "wapply.h"

typedef struct __WindowMany {
    G2Index g2index;

    size_t element_size;
    size_t size;
    size_t number;
    RWindow* _;
} __WindowMany;

MAKEMANY(RWindowMany);

IndexMC windowmany_count(RWindowMany);

MinMax windowmany_minmax_config(RWindowMany, const size_t);
MANY(MinMax) windowmany_minmax(RWindowMany windowmany);

void windowmany_shuffle(RWindowMany);

void windowmany_buildby_size(RWindowMany, size_t);
void windowmany_buildby_window0many(RWindowMany, RWindow0Many);
void windowmany_buildby_windowing(RWindowMany windowmany);

void windowmany_hash_update(SHA256_CTX*, RWindowMany);

#endif