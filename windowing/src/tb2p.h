#ifndef __WINDOWINGTESTBED2P_H__
#define __WINDOWINGTESTBED2P_H__

#include "common.h"

#include "windowsplit.h"
#include "tb2w.h"

#include <linux/limits.h>

typedef RWindowSplit TB2SBy_try;

MAKEMANY(TB2SBy_try);

typedef struct TB2SBy_split {
    MANY(TB2SBy_try) bytry;
} TB2SBy_split;


MAKEMANY(TB2SBy_split);

typedef struct __TB2S {
    RTB2W tb2w;

    MANY(WindowSplitConfig) splitconfigmany;

    MANY(TB2SBy_split) bysplit;
} __TB2S;

typedef const __TB2S TB2S;

RTB2S tb2p_create(RTB2W tb2w, size_t n_try, MANY(WindowSplitConfig) splitconfigmany);
void tb2p_run(RTB2S tb2s); 
void tb2p_free(RTB2S);

#endif