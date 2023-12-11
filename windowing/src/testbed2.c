
#include "testbed2.h"

#include "dataset.h"
#include "gatherer.h"
#include "io.h"
#include "stratosphere.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void testbed2_md(char dirname[200], const RTestBed2 tb2);

RTestBed2 testbed2_create(MANY(WSize) wsizes) {
    RTestBed2 tb2 = calloc(1, sizeof(__TestBed2));
    memset(tb2, 0, sizeof(__TestBed2));

    CLONEMANY(tb2->wsizes, wsizes);
    INITMANY(tb2->datasets.bywsize, wsizes.number, RDataset0);

    return tb2;
}

size_t _testbed2_get_index(MANY(RSource) sources) {
    size_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
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

    INITMANY(tb2->windowings.bysource, tb2->sources.number, TB2WindowingsSource);

    int32_t count_windows[tb2->wsizes.number][N_DGACLASSES];
    memset(&count_windows, 0, sizeof(int32_t) * tb2->wsizes.number * N_DGACLASSES);
    for (size_t s = 0; s < tb2->sources.number; s++) {
        RSource source = tb2->sources._[s];
        MANY(RWindowing)* windowings = &tb2->windowings.bysource._[s].bywsize;

        INITMANYREF(windowings, tb2->wsizes.number, RWindowing);

        for (size_t u = 0; u < tb2->wsizes.number; u++) {
            const WSize wsize = tb2->wsizes._[u];
            windowings->_[u] = windowings_create(wsize, source);

            count_windows[u][source->wclass.mc] += windowings->_[u]->windows.number;
        }
    }

    for (size_t u = 0; u < tb2->wsizes.number; u++) {
        MANY(RWindowing) tmp;

        INITMANY(tmp, tb2->sources.number, RWindowing);

        for (size_t s = 0; s < tb2->sources.number; s++) {
            tmp._[s] = tb2->windowings.bysource._[s].bywsize._[u];
        }

        tb2->datasets.bywsize._[u] = dataset0_from_windowings(tmp);

        FREEMANY(tmp);
    }
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

    for (size_t s = 0; s < tb2->sources.number; s++) {
        TB2WindowingsSource swindowings = tb2->windowings.bysource._[s];
        for (size_t i = 0; i < tb2->wsizes.number; i++) {
            for (size_t w = 0; w < swindowings.bywsize._[i]->windows.number; w++) {
                wapply_init(swindowings.bywsize._[i]->windows._[w], tb2->applies.number);
            }
        }
    }

    printf("New psets added over %ld: %ld\n", old_applies_count, new_applies_count);
}

void testbed2_apply(RTestBed2 tb2) {
    MANY(PSet) to_apply_psets;

    if (tb2->applies.number == 0) {
        printf("Error: impossible to apply, no parameters added.\n");
        return;
    }
    {
        size_t applied = 0;
        for (size_t a = 0; a < tb2->applies.number; a++) {
            applied += tb2->applies._[a].applied;
        }
        if (applied == tb2->applies.number) {
            printf("Notice: all applies already applied, skipping apply.\n");
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


    for (size_t s = 0; s < tb2->sources.number; s++) {
        TB2WindowingsSource swindowings = tb2->windowings.bysource._[s];
        stratosphere_apply(swindowings.bywsize, &to_apply_psets);
    }

    for (size_t p = 0; p < tb2->applies.number; p++) {
        tb2->applies._[p].applied = 1;
    }

    FREEMANY(to_apply_psets);
}

void testbed2_free(RTestBed2 tb2) {
    for (size_t s = 0; s < tb2->sources.number; s++) {
        FREEMANY(tb2->windowings.bysource._[s].bywsize);
    }
    FREEMANY(tb2->folds);
    FREEMANY(tb2->windowings.bysource);
    FREEMANY(tb2->datasets.bywsize);
    FREEMANY(tb2->sources);
    FREEMANY(tb2->wsizes);
    FREEMANY(tb2->applies);
    free(tb2);
}

void testbed2_io_parameters(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    MANY(TestBed2Apply)* applies = &tb2->applies;

    FRW(applies->number);

    if (rw == IO_READ && applies->number > 0) {
        INITMANYREF(applies, applies->number, TestBed2Apply);
    }

    for (size_t p = 0; p < applies->number; p++) {
        TestBed2Apply* apply = &applies->_[p];

        FRW(apply->applied);

        FRW(apply->pset.index);
        FRW(apply->pset.ninf);
        FRW(apply->pset.pinf);
        FRW(apply->pset.nn);
        FRW(apply->pset.wl_rank);
        FRW(apply->pset.wl_value);
        FRW(apply->pset.windowing);
        FRW(apply->pset.nx_epsilon_increment);
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

    for (size_t w = 0; w < windowing->windows.number; w++) {
        RWindow0 window0 = windowing->windows._[w];

        window0->windowing = windowing;

        FRW(window0->applies_number);
        FRW(window0->duration);
        FRW(window0->fn_req_min);
        FRW(window0->fn_req_max);

        if (rw == IO_READ) {
            wapply_init(window0, tb2->applies.number);
        }
        for (size_t p = 0; p < tb2->applies.number; p++) {
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
        INITMANY(tb2->windowings.bysource, tb2->sources.number, TB2WindowingsSource);
    }

    for (size_t i = 0; i < tb2->windowings.bysource.number; i++) {
        if (rw == IO_READ) {
            INITMANY(tb2->windowings.bysource._[i].bywsize, tb2->wsizes.number, RWindowing);
        }
    
        for (size_t w = 0; w < tb2->wsizes.number; w++) {
            RWindowing* windowing_ref;
            SourceIndex source_index;
            WSize wsize;

            windowing_ref = &tb2->windowings.bysource._[i].bywsize._[w];

            if (rw == IO_WRITE) {
                source_index = (*windowing_ref)->source->index;
                wsize = (*windowing_ref)->wsize;
            }

            FRW(source_index);
            FRW(wsize);

            if (rw == IO_READ) {
                (*windowing_ref) = windowings_create(wsize, tb2->sources._[source_index.all]);
            }

            FRW((*windowing_ref)->windows.number);

            testbed2_io_windowing_windows(rw, file, tb2, *windowing_ref);
        }
    }
}

void testbed2_io_dataset(IOReadWrite rw, FILE* file, RTestBed2 tb2, const WSize wsize, RDataset0* ds_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    WSize wsize2;
    Index windows_counter;

    if (rw == IO_WRITE) {
        wsize2 = wsize;
        windows_counter = dataset_counter(*ds_ref);
    }

    FRW(wsize2);
    FRW(windows_counter);

    if (wsize2.index != wsize.index && wsize2.value != wsize.value) {
        printf("[io:%d] Error wsize index not match\n", __LINE__);
    }

    if (rw == IO_READ) {
        (*ds_ref) = dataset0_create(wsize, windows_counter);
    }

    Index walker;
    memset(&walker, 0, sizeof(Index));
    for (size_t i = 0; i < windows_counter.all; i++) {
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
            (*window_ref) = tb2->windowings.bysource._[source_index].bywsize._[wsize_index]->windows._[window_index];
            (*window_bin_ref) = (*window_ref);
            (*window_mul_ref) = (*window_ref);
        }
    }
}

void testbed2_io_folds(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(tb2->folds.number);

    if (rw == IO_READ) {
        INITMANY(tb2->folds, tb2->folds.number, RFold);
    }

    for (size_t f = 0; f < tb2->folds.number; f++) {
        fold_io(rw, file, tb2, &tb2->folds._[f]);
    }
}

int testbed2_io(IOReadWrite rw, char dirname[200], RTestBed2* tb2) {
    char fpath[210];
    FILE* file;

    if (io_makedir(dirname, 200, 0)) {
        printf("Error: impossible to create directory <%s>\n", dirname);
        return -1;
    }

    sprintf(fpath, "%s/tb2.bin", dirname);
    file = io_openfile(rw, fpath);

    if (file == NULL) {
        printf("Error: file <%s> not opened.\n", fpath);
        return -1;
    }

    if (rw == IO_READ) {
        MANY(WSize) wsizes;
        testbed2_io_wsizes(rw, file, &wsizes);
        *tb2 = testbed2_create(wsizes);
        FREEMANY(wsizes);
    } else {
        testbed2_io_wsizes(rw, file, &(*tb2)->wsizes);
    }

    testbed2_io_sources(rw, file, (*tb2));

    testbed2_io_parameters(rw, file, (*tb2));

    testbed2_io_windowing(rw, file, (*tb2));

    for (size_t i = 0; i < (*tb2)->datasets.bywsize.number; i++) {
        testbed2_io_dataset(rw, file, (*tb2), (*tb2)->wsizes._[i], &(*tb2)->datasets.bywsize._[i]);
    }

    testbed2_io_folds(rw, file, (*tb2));

    fclose(file);

    if (rw == IO_WRITE) {
        testbed2_md(dirname, *tb2);
    }

    return 0;
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
    size_t applies_applied_count = 0;
    {
        const int len_header = 18;
        const char* headers[] = {
            "id",
            "applied",
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
        for (size_t i = 0; i < tb2->applies.number; i++) {
            TCPC(PSet) pset = &tb2->applies._[i].pset;
            applies_applied_count += tb2->applies._[i].applied;
            FP("|%*ld", len_header, i);
            FP("|%*d", len_header,  tb2->applies._[i].applied);
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

    FPNL(2, "> This TestBed has applied for %ld over %ld applies.", applies_applied_count, tb2->applies.number);

    FPNL(2, "## Foldings");

    for (size_t f = 0; f < tb2->folds.number; f++) {

        FPNL(2, "## Fold %ld", f);
        
        fold_md(file, tb2->folds._[f]);
    }

    fclose(file);

    #undef FP
    #undef FPNL
}