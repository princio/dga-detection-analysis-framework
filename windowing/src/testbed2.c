
#include "testbed2.h"

#include "dataset.h"
#include "gatherer.h"
#include "io.h"
#include "logger.h"
#include "stratosphere.h"
#include "testbed2_io.h"

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

    tb2->sources.element_size = sizeof(RSource);
    tb2->dataset.folds.element_size = sizeof(FoldConfig);

    INITBY_N(tb2->dataset, wsize, wsizes.number);
    INITBY_N(tb2->dataset, try, n_try);

    {
        size_t enabled = 0;
        for (size_t idxconfig = 0; idxconfig < configsuite.number; idxconfig++) {
            if (!configsuite._[idxconfig].disabled) {
                enabled++;
            }
        }

        size_t a = 0;
        INITMANY(tb2->applies, enabled, ConfigApplied);
        for (size_t idxconfig = 0; idxconfig < configsuite.number; idxconfig++) {
            if (configsuite._[idxconfig].disabled) {
                continue;
            }
            tb2->applies._[a].applied = 0;
            tb2->applies._[a].index = a;
            tb2->applies._[a].config = &configsuite._[idxconfig];
            a++;
        }
    }
    
    
    INITBY1(tb2->dataset, wsize, TestBed2DatasetBy);
    FORBY(tb2->dataset, wsize) {
        INITBY2(tb2->dataset, wsize, try, TestBed2DatasetBy);
        FORBY(tb2->dataset, try) {
            GETBY2(tb2->dataset, wsize, try).byfold.element_size = sizeof(TestBed2DatasetBy_fold);
        }
    }
    
    return tb2;
}

void testbed2_source_add(RTestBed2 tb2, RSource source) {
    GATHERER_ADD(tb2->sources, 5, source, RSource);

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
    INITBY1(tb2->windowing, wsize, TestBed2WindowingBy);
    FORBY(tb2->windowing, wsize) {
        INITBY2(tb2->windowing, wsize, source, TestBed2WindowingBy);
    }

    int32_t count_windows[tb2->wsizes.number][N_DGACLASSES];
    memset(&count_windows, 0, sizeof(int32_t) * tb2->wsizes.number * N_DGACLASSES);
    FORBY(tb2->windowing, wsize) {
        const WSize wsize = tb2->wsizes._[idxwsize];
        FORBY(tb2->windowing, source) {
            RSource source;
            RWindowing* windowing_ref;

            source = tb2->sources._[idxsource];
            windowing_ref = &GETBY2(tb2->windowing, wsize, source);

            *windowing_ref = windowings_create(wsize, source);
            count_windows[idxwsize][source->wclass.mc] += (*windowing_ref)->windows.number;
        }
    }

    FORBY(tb2->dataset, wsize) {
        FORBY(tb2->dataset, try) {
            GETBY2(tb2->dataset, wsize, try).dataset = dataset_from_windowings(GETBY(tb2->windowing, wsize).bysource);
            dataset_shuffle(GETBY2(tb2->dataset, wsize, try).dataset);
        }
    }

    FORBY(tb2->windowing, wsize) {
        FORBY(tb2->windowing, source) {
            RWindowing windowing = GETBY2(tb2->windowing, wsize, source);
            for (size_t w = 0; w < windowing->windows.number; w++) {
                // wapply_init(windowing->windows._[w]);
                INITMANY(windowing->windows._[w]->applies, configsuite.number, WApply);
            }
        }
    }
}

void testbed2_fold_add(RTestBed2 tb2, FoldConfig config) {
    const size_t n_old_folds = tb2->dataset.n.fold;
    GATHERER_ADD(tb2->dataset.folds, 5, config, FoldConfig);
    INITBY_N(tb2->dataset, fold, tb2->dataset.folds.number);

    FORBY(tb2->dataset, wsize) {
        FORBY(tb2->dataset, try) {
            FORBY(tb2->dataset, fold) {
                if (idxfold < n_old_folds) continue;
                DatasetSplits splits = dataset_splits(GETBY2(tb2->dataset, wsize, try).dataset, config.k, config.k_test);
                GATHERER_ADD(GETBY2(tb2->dataset, wsize, try).byfold, 5, splits, DatasetSplits);
            }
        }
    }
    size_t n_fold = tb2->dataset.n.fold;
    INITBY_N(tb2->dataset, fold, n_fold);
}

int testbed2_try_set(RTestBed2 tb2, size_t n_try) {
    const size_t n_try_old = tb2->dataset.n.try;

    if (n_try <= n_try_old) {
        LOG_WARN("Already %ld tries.", n_try);
        return -1;
    }

    INITBY_N(tb2->dataset, try, n_try);

    FORBY(tb2->dataset, wsize) {
        MANY(TestBed2DatasetBy_try) tb2_dataset_try_new;
        INITMANY(tb2_dataset_try_new, n_try, TestBed2DatasetBy_try);

        FORBY(tb2->dataset, try) {

            if (idxtry < n_try_old) {
                tb2_dataset_try_new._[idxtry] = GETBY2(tb2->dataset, wsize, try);
            } else {
                INITMANY(tb2_dataset_try_new._[idxtry].byfold, tb2->dataset.n.fold, TestBed2DatasetBy_fold);

                tb2_dataset_try_new._[idxtry].dataset = dataset_from_windowings(GETBY(tb2->windowing, wsize).bysource);
                dataset_shuffle(tb2_dataset_try_new._[idxtry].dataset);

                FORBY(tb2->dataset, fold) {
                    tb2_dataset_try_new._[idxtry].byfold._[idxfold] = dataset_splits(tb2_dataset_try_new._[idxtry].dataset, tb2->dataset.folds._[idxfold].k, tb2->dataset.folds._[idxfold].k_test);
                }
            }
        }

        tb2->dataset.bywsize._[idxwsize].bytry = tb2_dataset_try_new;
    }

    return 0;
}

/*
void testbed2_addpsets(RTestBed2 tb2) {
    size_t new_applies_count;
    size_t new_psets_index;
    int new_psets[configsuite.number];

    const size_t old_applies_count = tb2->applies.number;

    {
        memset(new_psets, 0, sizeof(int) * configsuite.number);
        new_applies_count = 0;
        for (size_t new = 0; new < configsuite.number; new++) {
            int is_new = 1;
            for (size_t old = 0; old < tb2->applies.number; old++) {
                int cmp = memcmp(&configsuite._[new], &tb2->applies._[old].config, sizeof(Config));
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
    tb2->applies._ = realloc(tb2->applies._, sizeof(ConfigApplied) * tb2->applies.number);

    new_psets_index = old_applies_count;
    for (size_t a = 0; a < configsuite.number; a++) {
        if (new_psets[a]) {
            tb2->applies._[new_psets_index].config = configsuite._[a];
            tb2->applies._[new_psets_index].applied = 0;
            new_psets_index++;
        }
    }

    FORBY(tb2->windowing, wsize) {
        FORBY(tb2->windowing, source) {
            RWindowing windowing = GETBY2(tb2->windowing, wsize, source);
            for (size_t w = 0; w < windowing->windows.number; w++) {
                INITMANY(windowing->windows._[w]->applies, tb2->applies.number, WApply);
            }
        }
    }

    LOG_INFO("New psets added over %ld: %ld.", old_applies_count, new_applies_count);
}
*/

void testbed2_apply(RTestBed2 tb2) {
    MANY(RWindowing) source_windowings;

    INITMANY(source_windowings, tb2->wsizes.number, RWindowing);

    FORBY(tb2->windowing, source) {


        size_t total_applies_undone = 0;

        FORBY(tb2->windowing, wsize) {

            for (size_t idxapply = 0; idxapply < tb2->applies.number; idxapply++) {
                const int applied = testbed2_io_windowing_applied(tb2, GETBY2(tb2->windowing, wsize, source), idxapply);

                tb2->applies._[idxapply].applied = applied;

                if (0 == applied) {
                    total_applies_undone++;
                } else {
                    testbed2_io_windowing_apply_windows_file(IO_READ, tb2, GETBY2(tb2->windowing, wsize, source), idxapply);
                }

                LOG_DEBUG("Apply %sexist for apply %ld (config %ld)", applied ? "" : "not ", idxapply, tb2->applies._[idxapply].config->index);
            }

            source_windowings._[idxwsize] = GETBY2(tb2->windowing, wsize, source);
        }

        LOG_DEBUG("Performing %ld applies for source %ld having fnreqmax=%ld.", total_applies_undone, idxsource, tb2->sources._[idxsource]->fnreq_max);
        stratosphere_apply(source_windowings, tb2->applies);

        FORBY(tb2->windowing, wsize) {
            for (size_t idxapply = 0; idxapply < tb2->applies.number; idxapply++) {
                if (tb2->applies._[idxapply].applied) {
                    testbed2_io_windowing_apply_windows_file(IO_WRITE, tb2, GETBY2(tb2->windowing, wsize, source), idxapply);
                    tb2->applies._[idxapply].applied = 1;
                }
            }
        }
    }

    FREEMANY(source_windowings);
}

void testbed2_free(RTestBed2 tb2) {
    FORBY(tb2->windowing, wsize) {
        FREEMANY(GETBY(tb2->windowing, wsize).bysource);
    }
    FREEMANY(tb2->windowing.bywsize);

    FORBY(tb2->dataset, wsize) {
        FORBY(tb2->dataset, try) {
            FORBY(tb2->dataset, fold) {
                FREEMANY(GETBY3(tb2->dataset, wsize, try, fold).splits);
            }
            FREEMANY(GETBY2(tb2->dataset, wsize, try).byfold);
        }
        FREEMANY(GETBY(tb2->dataset, wsize).bytry);
    }
    FREEMANY(tb2->dataset.bywsize);
    FREEMANY(tb2->dataset.folds);

    FREEMANY(tb2->sources);
    FREEMANY(tb2->wsizes);
    FREEMANY(tb2->applies);
    free(tb2);
}
