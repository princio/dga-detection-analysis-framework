
#ifndef __REDUCER_H__
#define __REDUCER_H__

#include "common.h"

typedef struct Reducer {
    size_t n_blocks;
    size_t n_logit_max;
    int64_t reducer;
    MANY(int64_t) many;
} Reducer;

int64_t reducer_logit(double logit, const int reducer);

Reducer reducer_run(RTB2W tb2w, const size_t nblocks, const size_t nlogits_max, const int64_t reducer);

#endif