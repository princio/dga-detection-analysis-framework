
#include "tb2d.h"

RTB2D tb2d_create(RTB2W tb2w, size_t n_try, MANY(FoldConfig) foldconfigs) {
    RTB2D tb2d;

    tb2d = calloc(1, sizeof(__TB2D));

    tb2d->tb2w = tb2w;

    BY_SETN(*tb2d, try, n_try);
    BY_SETN(*tb2d, fold, foldconfigs.number);

    MANY_CLONE(tb2d->folds, foldconfigs);

    BY_INIT1(*tb2d, try, TB2DBy);
    BY_FOR(*tb2d, try) {
        BY_INIT2(*tb2d, try, fold, TB2DBy);
    }

    return tb2d;
}

void tb2d_run(RTB2D tb2d) {
    BY_FOR(*tb2d, try) {
        BY_GET(*tb2d, try).dataset = dataset_from_windowings(tb2d->tb2w->windowing.bysource, tb2d->tb2w->configsuite.configs.number);

        dataset_shuffle(BY_GET(*tb2d, try).dataset);

        BY_FOR(*tb2d, fold) {
            BY_GET2(*tb2d, try, fold) = dataset_splits(BY_GET(*tb2d, try).dataset, tb2d->folds._[idxfold].k, tb2d->folds._[idxfold].k_test);
        }
    }
}

void tb2d_free(RTB2D tb2d) {
    BY_FOR((*tb2d), try) {
        BY_FOR((*tb2d), fold) {
            MANY_FREE(BY_GET2((*tb2d), try, fold).splits);
        }
        MANY_FREE(BY_GET((*tb2d), try).byfold);
    }
    MANY_FREE(tb2d->bytry);
    MANY_FREE(tb2d->folds);
    free(tb2d);
}
