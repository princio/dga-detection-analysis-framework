
#include "tb2p.h"

#include "windowsplit.h"
#include "windowmc.h"

RTB2S tb2p_create(RTB2W tb2w, size_t n_try, MANY(WindowSplitConfig) splitconfigmany) {
    RTB2S tb2p;

    tb2p = calloc(1, sizeof(__TB2S));

    tb2p->tb2w = tb2w;

    MANY_CLONE(tb2p->splitconfigmany, splitconfigmany);

    MANY_INIT(tb2p->bysplit, splitconfigmany.number, TB2SBy_split);

    MANY_FOR(tb2p->bysplit, split) {
        WindowSplitConfig splitconfig;

        splitconfig = tb2p->splitconfigmany._[idxsplit];

        MANY_INIT(tb2p->bysplit._[idxsplit].bytry, (
            splitconfig.how == WINDOWSPLIT_HOW_PORTION ? splitconfig.try : 1
        ), TB2SBy_split);
    }

    return tb2p;
}

void tb2p_run(RTB2S tb2p) {
    MANY_FOR(tb2p->bysplit, split) {
        WindowSplitConfig splitconfig = tb2p->splitconfigmany._[idxsplit];

        if (splitconfig.how == WINDOWSPLIT_HOW_BY_DAY) {
            tb2p->bysplit._[idxsplit].bytry._[0] = windowsplit_by_day(tb2p->tb2w->windowing.bysource, splitconfig.day);
        } else
        if (splitconfig.how == WINDOWSPLIT_HOW_PORTION) {
            RWindowMC windowmc;
            
            windowmc = windowmc_alloc_by_windowingmany(tb2p->tb2w->windowing.bysource);

            MANY_FOR(tb2p->bysplit._[idxsplit].bytry, try) {
                BY_GET2(*tb2p, split, try) = windowsplit_by_portion(windowmc, splitconfig.portion);
                windowmc_shuffle(windowmc);
            }
        } else {
            exit(1);
        }
    }
}

void tb2p_free(RTB2S tb2p) {
    MANY_FOR(tb2p->bysplit, split) {
        MANY_FREE(MANY_GET((*tb2p), split).bytry);
    }
    MANY_FREE(tb2p->bysplit);
    MANY_FREE(tb2p->splitconfigmany);
    free(tb2p);
}
