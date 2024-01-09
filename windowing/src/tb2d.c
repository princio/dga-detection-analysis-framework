
#include "tb2d.h"

RTB2D tb2d_generate(RTB2W tb2w, size_t n_try, MANY(FoldConfig) foldconfigs) {
    TB2D tb2d;
    RTB2D rtb2d;

    memset(&tb2d, 0, sizeof(TB2D));
    rtb2d = calloc(1, sizeof(TB2D));

    tb2d.tb2w = tb2w;

    BY_SETN(tb2d, try, n_try);
    BY_SETN(tb2d, fold, foldconfigs.number);

    MANY_INIT(tb2d.bytry, n_try, TB2DBy_try);

    tb2d.folds = foldconfigs;

    BY_FOR(tb2d, try) {
        BY_INIT1(tb2d, try, TB2DBy);

        BY_GET(tb2d, try).dataset = dataset_from_windowings(tb2w->windowing.bysource);

        dataset_shuffle(BY_GET(tb2d, try).dataset);

        BY_FOR(tb2d, fold) {
            BY_GET2(tb2d, try, fold) = dataset_splits(BY_GET(tb2d, try).dataset, tb2d.folds._[idxfold].k, tb2d.folds._[idxfold].k_test);
        }
    }

    return rtb2d;
}

void tb2d_free(RTB2D tb2d) {
    FREEMANY(tb2d->folds);
    BY_FOR((*tb2d), try) {
        BY_FOR((*tb2d), fold) {
            FREEMANY(BY_GET2((*tb2d), try, fold).splits);
        }
        FREEMANY(BY_GET((*tb2d), try).byfold);
    }
    FREEMANY(tb2d->bytry);
    free(tb2d);
}
