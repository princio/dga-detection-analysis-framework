
#include "testbed2.h"

#include "dataset.h"
#include "gatherer.h"
#include "io.h"
#include "logger.h"
#include "stratosphere.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

size_t _testbed2_get_index(MANY(RSource) sources) {
    size_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
}

RTestBed2 testbed2_create(MANY(WSize) wsizes, const size_t n_try) {
    RTestBed2 tb2 = calloc(1, sizeof(__TestBed2));
    memset(tb2, 0, sizeof(__TestBed2));

    CLONEMANY(tb2->wsizes, wsizes);

    INITBY_N(tb2->dataset, wsize, wsizes.number);
    INITBY_N(tb2->dataset, try, n_try);
    
    INITBY(tb2->dataset, tb2->dataset, wsize, TestBed2DatasetBy);
    FORBY(tb2->dataset, wsize) {
        INITBY(tb2->dataset, GETBY(tb2->dataset, wsize), try, TestBed2DatasetBy);
        FORBY(tb2->dataset, try) {
            INITMANY(GETBY(GETBY(tb2->dataset, wsize), try).byfold, 10, TestBed2DatasetBy_fold);
            GETBY(GETBY(tb2->dataset, wsize), try).byfold.number = 0;
        }
    }
    
    return tb2;
}

void testbed2_source_add(RTestBed2 tb2, RSource source) {
    gatherer_many_add((__MANY*) &tb2->sources, sizeof(RSource), 5, source);

    SourceIndex walker;
    memset(&walker, 0, sizeof(SourceIndex));
    for (size_t i = 0; i < tb2->sources.number - 1; i++) {
        walker.all++;
        walker.binary += tb2->sources._[i]->wclass.bc == source->wclass.bc;
        walker.multi += tb2->sources._[i]->wclass.mc == source->wclass.mc;
    }

    source->index = walker;
}

void testbed2_windowing(RTestBed2 tb2) {
    gatherer_many_finalize((__MANY*) &tb2->sources, sizeof(RSource));

    INITBY_N(tb2->windowing, source, tb2->sources.number);
    INITBY_N(tb2->windowing, wsize, tb2->wsizes.number);
    INITBY(tb2->windowing, tb2->windowing, source, TestBed2WindowingBy);
    FORBY(tb2->windowing, source) {
        INITBY(tb2->windowing, GETBY(tb2->windowing, source), wsize, TestBed2WindowingBy);
    }

    int32_t count_windows[tb2->wsizes.number][N_DGACLASSES];
    memset(&count_windows, 0, sizeof(int32_t) * tb2->wsizes.number * N_DGACLASSES);
    for (size_t s = 0; s < tb2->sources.number; s++) {
        RSource source = tb2->sources._[s];
        MANY(RWindowing)* windowings = &tb2->windowing.bysource._[s].bywsize;

        INITMANYREF(windowings, tb2->wsizes.number, RWindowing);

        for (size_t u = 0; u < tb2->wsizes.number; u++) {
            const WSize wsize = tb2->wsizes._[u];
            windowings->_[u] = windowings_create(wsize, source);

            count_windows[u][source->wclass.mc] += windowings->_[u]->windows.number;
        }
    }

    FORBY(tb2->dataset, wsize) {
        MANY(RWindowing) tmp;

        INITMANY(tmp, tb2->sources.number, RWindowing);

        FORBY(tb2->windowing, source) {
            tmp._[idxsource] = GETBY2(tb2->windowing, source, wsize);
        }

        FORBY(tb2->dataset, try) {
            GETBY2(tb2->dataset, wsize, try).dataset = dataset0_from_windowings(tmp);
        }

        FREEMANY(tmp);
    }
}

void testbed2_fold_add(RTestBed2 tb2, FoldConfig config) {
    gatherer_many_add((__MANY*) &tb2->dataset.folds, sizeof(FoldConfig), 5, &config);

    FORBY(tb2->dataset, wsize) {
        FORBY(tb2->dataset, try) {
            FORBY(tb2->dataset, fold) {
                DatasetSplits splits = dataset0_splits(GETBY2(tb2->dataset, wsize, try).dataset, config.k, config.k_test);
                gatherer_many_add((__MANY*) &GETBY2(tb2->dataset, wsize, try).byfold, sizeof(TestBed2DatasetBy), 5, &splits);
            }
        }
    }
    size_t n_fold = 1 + tb2->dataset.n.fold;
    INITBY_N(tb2->dataset, fold, n_fold);
}

void testbed2_addpsets(RTestBed2 tb2, MANY(PSet) psets) {
    size_t new_applies_count;
    size_t new_psets_index;
    int new_psets[psets.number];

    const size_t old_applies_count = tb2->applies.number;

    {
        memset(new_psets, 0, sizeof(int) * psets.number);
        new_applies_count = 0;
        for (size_t new = 0; new < psets.number; new++) {
            int is_new = 1;
            for (size_t old = 0; old < tb2->applies.number; old++) {
                int cmp = memcmp(&psets._[new], &tb2->applies._[old].pset, sizeof(PSet));
                if (cmp == 0) {
                    is_new = 0;
                    break;
                }
            }
            new_psets[new] = is_new;
            new_applies_count += is_new;
        }
    }

    tb2->applies.number = tb2->applies.number + new_applies_count;
    tb2->applies._ = realloc(tb2->applies._, sizeof(TestBed2Apply) * tb2->applies.number);

    new_psets_index = old_applies_count;
    for (size_t a = 0; a < psets.number; a++) {
        if (new_psets[a]) {
            tb2->applies._[new_psets_index].pset = psets._[a];
            tb2->applies._[new_psets_index].applied = 0;
            new_psets_index++;
        }
    }

    FORBY(tb2->windowing, source) {
        FORBY(tb2->windowing, wsize) {
            RWindowing windowing = tb2->windowing.bysource._[idxsource].bywsize._[idxwsize];
            for (size_t w = 0; w < windowing->windows.number; w++) {
                wapply_init(windowing->windows._[w], tb2->applies.number);
            }
        }
    }

    LOG_INFO("New psets added over %ld: %ld.", old_applies_count, new_applies_count);
}

void testbed2_apply(RTestBed2 tb2) {
    MANY(PSet) to_apply_psets;

    if (tb2->applies.number == 0) {
        LOG_ERROR("impossible to apply, no parameters added.");
        return;
    }
    {
        size_t applied = 0;
        for (size_t a = 0; a < tb2->applies.number; a++) {
            applied += tb2->applies._[a].applied;
        }
        if (applied == tb2->applies.number) {
            LOG_INFO("Notice: all applies already applied, skipping apply.\n");
            return;
        }
    }

    {
        int idx = 0;
        INITMANY(to_apply_psets, tb2->applies.number, PSet);
        for (size_t p = 0; p < tb2->applies.number; p++) {
            if (tb2->applies._[p].applied == 0) {
                to_apply_psets._[idx] = tb2->applies._[p].pset;
                ++idx;
            }
        }
        to_apply_psets.number = idx;
    }


    FORBY(tb2->windowing, source) {
        LOG_DEBUG("Performing apply for source %ld having fnreqmax=%ld.", idxsource, tb2->sources._[idxsource]->fnreq_max);
        stratosphere_apply(tb2->windowing.bysource._[idxsource].bywsize, &to_apply_psets);
    }

    for (size_t p = 0; p < tb2->applies.number; p++) {
        tb2->applies._[p].applied = 1;
    }

    FREEMANY(to_apply_psets);
}

void testbed2_free(RTestBed2 tb2) {
    FORBY(tb2->windowing, source) {
        FREEMANY(tb2->windowing.bysource._[idxsource].bywsize);
    }
    FORBY(tb2->dataset, wsize) {
        FORBY(tb2->dataset, try) {
            FORBY(tb2->dataset, fold) {
                FREEMANY(GETBY(GETBY(tb2->dataset, wsize), try).byfold);
            }
            FREEMANY(GETBY(GETBY(tb2->dataset, wsize), try).byfold);
        }
        FREEMANY(GETBY(tb2->dataset, wsize).bytry);
    }
    FREEMANY(tb2->windowing.bysource);
    FREEMANY(tb2->dataset.bywsize);
    FREEMANY(tb2->sources);
    FREEMANY(tb2->wsizes);
    FREEMANY(tb2->applies);
    free(tb2);
}
