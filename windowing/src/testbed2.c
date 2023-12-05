
#include "testbed2.h"

#include "cache.h"
#include "dataset.h"
#include "io.h"
#include "stratosphere.h"
#include "tetra.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

TestBed2* testbed2_create(MANY(WSize) wsizes) {
    TestBed2* tb2 = calloc(1, sizeof(TestBed2));
    memset(tb2, 0, sizeof(TestBed2));

    INITMANY(tb2->wsizes, wsizes.number, WSize);
    INITMANY(tb2->datasets.wsize, wsizes.number, RDataset0);

    for (size_t i = 0; i < tb2->wsizes.number; i++) {
        tb2->wsizes._[i].index = wsizes._[i].index;
        tb2->wsizes._[i].value = wsizes._[i].value;
    }

    return tb2;
}

size_t _testbed2_get_index(MANY(RSource) sources) {
    size_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
}

void testbed2_source_add(TestBed2* tb2, __Source* source) {
    sources_add(&tb2->sources, source);
}

void testbed2_windowing(TestBed2* tb2) {
    sources_finalize(&tb2->sources);

    INITMANY(tb2->windowings.source, tb2->sources.number, TB2WindowingsSource);

    int32_t count_windows[tb2->wsizes.number][N_DGACLASSES];
    memset(&count_windows, 0, sizeof(int32_t) * tb2->wsizes.number * N_DGACLASSES);
    for (size_t s = 0; s < tb2->sources.number; s++) {
        RSource source = tb2->sources._[s];
        MANY(RWindowing)* windowings = &tb2->windowings.source._[s].wsize;

        INITMANYREF(windowings, tb2->wsizes.number, RWindowing);

        for (size_t u = 0; u < tb2->wsizes.number; u++) {
            const WSize wsize = tb2->wsizes._[u];
            windowings->_[u] = windowings_alloc(source, wsize);

            count_windows[u][source->wclass.mc] += windowings->_[u]->windows.number;
        }
    }

    for (size_t u = 0; u < tb2->wsizes.number; u++) {
        RDataset0* dataset = &tb2->datasets.wsize._[u];

        MANY(RWindowing) tmp;

        INITMANY(tmp, tb2->sources.number, RWindowing);

        for (size_t s = 0; s < tb2->sources.number; s++) {
            tmp._[s] = tb2->windowings.source._[s].wsize._[u];
        }

        tb2->datasets.wsize._[u] = dataset0_from_windowings(tmp);

        FREEMANY(tmp);
    }
}

void testbed2_apply(TestBed2* tb2, TCPC(MANY(PSet)) psets) {
    for (size_t w = 0; w < tb2->datasets.wsize.number; w++) {
        tb2->datasets.wsize._[w]->applies_number = psets->number;
    }
    
    for (size_t s = 0; s < tb2->sources.number; s++) {
        TB2WindowingsSource swindowings = tb2->windowings.source._[s];
        for (size_t i = 0; i < tb2->wsizes.number; i++) {
            for (size_t w = 0; w < swindowings.wsize._[i]->windows.number; w++) {
                wapply_init(swindowings.wsize._[i]->windows._[w], psets->number);
            }
        }
        stratosphere_apply(swindowings.wsize, psets, NULL);
    }

    tb2->applied = 1;
    tb2->psets.number = psets->number;
    tb2->psets._ = psets->_;
}

void testbed2_free(TestBed2* tb2) {
    for (size_t s = 0; s < tb2->sources.number; s++) {
        FREEMANY(tb2->windowings.source._[s].wsize);
    }
    FREEMANY(tb2->windowings.source);
    FREEMANY(tb2->datasets.wsize);
    FREEMANY(tb2->sources);
    FREEMANY(tb2->wsizes);
    free(tb2);
}

void testbed2_io_parameters(IOReadWrite rw, FILE* file, TestBed2* tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    MANY(PSet)* psets = &tb2->psets;

    FRW(psets->number);

    if (rw == IO_READ) {
        INITMANYREF(psets, psets->number, PSet);
    }

    for (size_t p = 0; p < psets->number; p++) {
        PSet* pset = &psets->_[p];

        FRW(pset->id);
        FRW(pset->infinite_values.ninf);
        FRW(pset->infinite_values.pinf);
        FRW(pset->nn);
        FRW(pset->whitelisting);
        FRW(pset->windowing);
    }
}

void testbed2_io_wsizes(IOReadWrite rw, FILE* file, MANY(WSize)* wsizes) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(wsizes->number);

    if (rw == IO_READ) {
        INITMANYREF(wsizes, wsizes->number, WSize);
    }

    for (size_t t = 0; t < wsizes->number; t++) {
        FRW(wsizes->_[t]);
    }
}

void testbed2_io_sources(IOReadWrite rw, FILE* file, TestBed2* tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    MANY(RSource)* sources = &tb2->sources;

    FRW(sources->number);

    if (rw == IO_READ) {
        INITMANYREF(sources, sources->number, RSource);
    }

    for (size_t t = 0; t < sources->number; t++) {
        RSource source;
        if (rw == IO_READ) {
            source = sources_alloc();
        } else {
            source = sources->_[t];
        }

        FRW(sources->_[t]);

        if (rw == IO_READ) {
            sources_add(sources, source);
        }
    }
}

void testbed2_io_windowing_windows(IOReadWrite rw, FILE* file, TestBed2* tb2, RWindowing windowing) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(windowing->windows.number);

    if (rw == IO_READ) {
        windowing->windows = window0s_alloc(windowing->windows.number);
    }
    for (size_t w = 0; w < windowing->windows.number; w++) {
        RWindow0 window0 = windowing->windows._[w];
        FRW(window0->applies_number);
        FRW(window0->duration);
        FRW(window0->fn_req_min);
        FRW(window0->fn_req_max);
        window0->windowing = windowing;
        if (rw == IO_READ) {
            wapply_init(window0, tb2->psets.number);
        }
        for (size_t p = 0; p < tb2->psets.number; p++) {
            WApply* wapply = &window0->applies._[p];
            FRW(wapply->dn_bad_05);
            // FRW(wapply->dn_bad_0999);
            // FRW(wapply->dn_bad_099);
            // FRW(wapply->dn_bad_09);
            FRW(wapply->logit);
            FRW(wapply->pset_index);
            FRW(wapply->wcount);
            // FRW(wapply->whitelistened);
        }
    }
}

void testbed2_io_windowing(IOReadWrite rw, FILE* file, TestBed2* tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(tb2->windowings.source.number);

    if (rw == IO_READ) {
        INITMANY(tb2->windowings.source, tb2->windowings.source.number, TB2WindowingsSource);
    }

    for (size_t i = 0; i < tb2->windowings.source.number; i++) {
        if (rw == IO_READ) {
            INITMANY(tb2->windowings.source._[i].wsize, tb2->windowings.source._[i].wsize.number, RWindowing);
        }
        MANY(RWindowing)* windowings = &tb2->windowings.source._[i].wsize;
        for (size_t w = 0; w < windowings->number; w++) {
            RWindowing* windowing_ref = &windowings->_[w];

            if (rw == IO_READ) {
                Index source_index;
                RSource source;
                WSize wsize;

                FRW(source_index);
                FRW(wsize);

                source = tb2->sources._[source_index.all];

                (*windowing_ref) = windowings_alloc(source, wsize);
            } else {
                FRW((*windowing_ref)->source->index);
                FRW((*windowing_ref)->wsize);
            }

            testbed2_io_windowing_windows(rw, file, tb2, *windowing_ref);
        }
    }
}

void testbed2_io_dataset(IOReadWrite rw, FILE* file, TestBed2* tb2, const size_t wsize_index, RDataset0* ds_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    Dataset0Init init;

    if (rw == IO_WRITE) {
        init.applies_number = (*ds_ref)->applies_number;
        init.wsize = (*ds_ref)->wsize;
        init.windows_counter = dataset_counter(*ds_ref);
    }

    FRW(init);

    if (rw == IO_READ) {
        if (init.wsize.index != wsize_index) {
            printf("[io:%d] Error wsize index not match\n", __LINE__);
        }
        (*ds_ref) = dataset0_alloc(init);
    }

    MANY(TB2WindowingsSource)* swindowings = &tb2->windowings.source;
    for (size_t s = 0; s < swindowings->number; s++) {
        RWindowing* windowing_ref = &swindowings->_[s].wsize._[wsize_index];

        if (rw == IO_READ) {
            Index source_index;
            RSource source;
            WSize wsize;

            FRW(source_index);
            FRW(wsize);

            source = tb2->sources._[source_index.all];

            (*windowing_ref) = windowings_alloc(source, wsize);
        } else {
            FRW((*windowing_ref)->source->index);
            FRW((*windowing_ref)->wsize.index);
            FRW((*windowing_ref)->wsize.value);
        }

        testbed2_io_windowing_windows(rw, file, tb2, *windowing_ref);
    }
}

void testbed2_io(IOReadWrite rw, char dirname[200], TestBed2* tb2) {
    char fpath[210];
    sprintf(fpath, "%s/tb2.bin", dirname);

    FILE* file = io_openfile(rw, fpath);

    if (file == NULL) {
        printf("Error: file <%s> not opened.\n", fpath);
        return;
    }


    if (rw == IO_READ) {
        MANY(WSize) wsizes;
        testbed2_io_wsizes(rw, file, &wsizes);
        tb2 = testbed2_create(wsizes);
    } else {
        testbed2_io_wsizes(rw, file, &tb2->wsizes);
    }

    testbed2_io_parameters(rw, file, tb2);

    testbed2_io_sources(rw, file, tb2);
    
    for (size_t i = 0; i < tb2->datasets.wsize.number; i++) {
        testbed2_io_dataset(rw, file, tb2, i, &tb2->datasets.wsize._[i]);
    }

    fclose(file);
}