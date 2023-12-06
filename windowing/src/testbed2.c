
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

void testbed2_md(char dirname[200], const RTestBed2 tb2);

RTestBed2 testbed2_create(MANY(WSize) wsizes) {
    RTestBed2 tb2 = calloc(1, sizeof(__TestBed2));
    memset(tb2, 0, sizeof(__TestBed2));

    CLONEMANY(tb2->wsizes, wsizes, WSize);
    INITMANY(tb2->datasets.wsize, wsizes.number, RDataset0);

    return tb2;
}

size_t _testbed2_get_index(MANY(RSource) sources) {
    size_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
}

void testbed2_source_add(RTestBed2 tb2, __Source* source) {
    sources_add(&tb2->sources, source);
}

void testbed2_windowing(RTestBed2 tb2) {
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

void testbed2_apply(RTestBed2 tb2, MANY(PSet) psets) {
    for (size_t w = 0; w < tb2->datasets.wsize.number; w++) {
        tb2->datasets.wsize._[w]->applies_number = psets.number;
    }
    
    for (size_t s = 0; s < tb2->sources.number; s++) {
        TB2WindowingsSource swindowings = tb2->windowings.source._[s];
        for (size_t i = 0; i < tb2->wsizes.number; i++) {
            for (size_t w = 0; w < swindowings.wsize._[i]->windows.number; w++) {
                wapply_init(swindowings.wsize._[i]->windows._[w], psets.number);
            }
        }
        stratosphere_apply(swindowings.wsize, &psets, NULL);
    }

    tb2->applied = 1;

    CLONEMANY(tb2->psets, psets, PSet);
}

void testbed2_free(RTestBed2 tb2) {
    for (size_t s = 0; s < tb2->sources.number; s++) {
        FREEMANY(tb2->windowings.source._[s].wsize);
    }
    FREEMANY(tb2->windowings.source);
    FREEMANY(tb2->datasets.wsize);
    FREEMANY(tb2->sources);
    FREEMANY(tb2->wsizes);
    FREEMANY(tb2->psets);
    free(tb2);
}

void testbed2_io_parameters(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    MANY(PSet)* psets = &tb2->psets;

    FRW(psets->number);

    if (rw == IO_READ) {
        INITMANYREF(psets, psets->number, PSet);
    }

    for (size_t p = 0; p < psets->number; p++) {
        PSet* pset = &psets->_[p];

        FRW(pset->id);
        FRW(pset->ninf);
        FRW(pset->pinf);
        FRW(pset->nn);
        FRW(pset->wl_rank);
        FRW(pset->wl_value);
        FRW(pset->windowing);
        FRW(pset->nx_epsilon_increment);
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

void testbed2_io_sources(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
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

        FRW(source->index);
        FRW(source->name);
        FRW(source->galaxy);
        FRW(source->wclass);
        FRW(source->id);
        FRW(source->qr);
        FRW(source->q);
        FRW(source->r);
        FRW(source->fnreq_max);

        if (rw == IO_READ) {
            sources->_[t] = source;
        }
    }
}

void testbed2_io_windowing_windows(IOReadWrite rw, FILE* file, RTestBed2 tb2, RWindowing windowing) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(windowing->windows.number);

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
            FRW(wapply->dn_bad_0999);
            FRW(wapply->dn_bad_099);
            FRW(wapply->dn_bad_09);
            FRW(wapply->logit);
            FRW(wapply->pset_index);
            FRW(wapply->wcount);
            FRW(wapply->whitelistened);
        }
    }
}

void testbed2_io_windowing(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    if (rw == IO_READ) {
        INITMANY(tb2->windowings.source, tb2->sources.number, TB2WindowingsSource);
    }

    for (size_t i = 0; i < tb2->windowings.source.number; i++) {
        if (rw == IO_READ) {
            INITMANY(tb2->windowings.source._[i].wsize, tb2->wsizes.number, RWindowing);
        }
        for (size_t w = 0; w < tb2->wsizes.number; w++) {
            RWindowing* windowing_ref = &tb2->windowings.source._[i].wsize._[w];

            if (rw == IO_READ) {
                SourceIndex source_index;
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

void testbed2_io_dataset(IOReadWrite rw, FILE* file, RTestBed2 tb2, const size_t wsize_index, RDataset0* ds_ref) {
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

    Index walker;
    memset(&walker, 0, sizeof(Index));
    for (size_t i = 0; i < init.windows_counter.all; i++) {
        RWindow0* window_ref = &(*ds_ref)->windows.all._[i];
        RWindow0* window_bin_ref;
        RWindow0* window_mul_ref;
        RSource source;

        size_t window_index;
        size_t source_index;
        size_t wsize_index;

        if (rw == IO_WRITE) {
            window_index = (*window_ref)->index;
            source_index = (*window_ref)->windowing->source->index.all;
            wsize_index = (*window_ref)->windowing->wsize.index;
        }

        FRW(window_index);
        FRW(source_index);
        FRW(wsize_index);

        source = tb2->sources._[source_index];

        window_bin_ref = &(*ds_ref)->windows.binary[source->wclass.bc]._[walker.binary[source->wclass.bc]++];
        window_mul_ref = &(*ds_ref)->windows.multi[source->wclass.mc]._[walker.multi[source->wclass.mc]++];

        if (rw == IO_READ) {
            (*window_ref) = tb2->windowings.source._[source_index].wsize._[wsize_index]->windows._[window_index];
            (*window_bin_ref) = (*window_ref);
            (*window_mul_ref) = (*window_ref);
        }
    }
}

void testbed2_io(IOReadWrite rw, char dirname[200], RTestBed2* tb2) {
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
        *tb2 = testbed2_create(wsizes);
        FREEMANY(wsizes);
    } else {
        testbed2_io_wsizes(rw, file, &(*tb2)->wsizes);
    }

    testbed2_io_parameters(rw, file, (*tb2));

    testbed2_io_sources(rw, file, (*tb2));

    testbed2_io_windowing(rw, file, (*tb2));
    
    for (size_t i = 0; i < (*tb2)->datasets.wsize.number; i++) {
        testbed2_io_dataset(rw, file, (*tb2), i, &(*tb2)->datasets.wsize._[i]);
    }

    fclose(file);

    if (rw == IO_WRITE) {
        testbed2_md(dirname, *tb2);
    }
}

void testbed2_md(char dirname[200], const RTestBed2 tb2) {
    char fpath[210];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    FILE* file;
    sprintf(fpath, "%s/tb2.md", dirname);
    file = fopen(fpath, "w");

    t = time(NULL);
    tm = *localtime(&t);

    #define FP(...) fprintf(file, __VA_ARGS__);
    #define FPNL(N, ...) fprintf(file, __VA_ARGS__); for (size_t i = 0; i < N; i++) fprintf(file, "\n");

    FPNL(2, "# TestBed");

    FPNL(3, "Saved on date: %d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    FPNL(2, "## WSizes");
    for (size_t i = 0; i < tb2->wsizes.number; i++) {
        FPNL(1, "- %ld", tb2->wsizes._[i].value);
    }

    FPNL(2, "## Sources");
    {
        const int len_header = 15;
        const char* headers[] = {
            "name",
            "galaxy",
            "wclass",
            "id",
            "qr",
            "q",
            "r",
            "fnreq_max"
        };
        const size_t n_headers = sizeof (headers) / sizeof (const char *);
        for (size_t i = 0; i < n_headers; i++) {
            FP("|%*s", len_header, headers[i]);
        }
        FPNL(1, "|");
        for (size_t i = 0; i < n_headers; i++) {
            FP("|");
            for (int t = 0; t < len_header; t++) {
                FP("-");
            }
        }
        FPNL(1, "|");
        for (size_t i = 0; i < tb2->sources.number; i++) {
            RSource source = tb2->sources._[i];
            FP("|%*s", len_header, source->name);
            FP("|%*s", len_header, source->galaxy);
            FP("|%*d", len_header, source->wclass.mc);
            FP("|%*d", len_header, source->id);
            FP("|%*ld", len_header, source->qr);
            FP("|%*ld", len_header, source->q);
            FP("|%*ld", len_header, source->r);
            FPNL(1, "|%*ld|", len_header, source->fnreq_max);
        }
    }

    FP("\n\n");

    FPNL(2, "## Parameters");
    {
        const int len_header = 18;
        const char* headers[] = {
            "id",
            "ninf",
            "pinf",
            "nn",
            "windowing",
            "nx eps increment",
            "rank",
            "value"
        };
        const size_t n_headers = sizeof (headers) / sizeof (const char *);
        for (size_t i = 0; i < n_headers; i++) {
            FP("|%*s", len_header, headers[i]);
        }
        FPNL(1, "|");
        for (size_t i = 0; i < n_headers; i++) {
            FP("|");
            for (int t = 0; t < len_header; t++) {
                FP("-");
            }
        }
        FPNL(1, "|");
        for (size_t i = 0; i < tb2->psets.number; i++) {
            TCPC(PSet) pset = &tb2->psets._[i];
            FP("|%*ld", len_header, pset->id);
            FP("|%*f", len_header,  pset->ninf);
            FP("|%*f", len_header,  pset->pinf);
            FP("|%*s", len_header,  NN_NAMES[pset->nn]);
            FP("|%*s", len_header,  WINDOWING_NAMES[pset->windowing]);
            FP("|%*f", len_header,  pset->nx_epsilon_increment);
            FP("|%*ld", len_header, pset->wl_rank);
            FPNL(1, "|%*f|", len_header, pset->wl_value);
        }
    }

    FP("\n\n");

    FPNL(2, "## Applied");

    if (tb2->applied == 0) {
        FPNL(2, "> This TestBed has not been applied.");
    } else {
        FPNL(2, "> This TestBed has been applied.");
    }

    #undef FP
    #undef FPNL
}