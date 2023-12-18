
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

void testbed2_md(char fpath[PATH_MAX], const RTestBed2 tb2);

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

        if (rw == IO_READ) {
            apply->pset.index = p;
        }

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

        FRW(window0->duration);

        struct {
            uint32_t min;
            uint32_t max;
        } fnreq;
        memset(&fnreq, 0, sizeof(fnreq));

        if (rw == IO_WRITE) {
            fnreq.min = window0->fn_req_min;
            fnreq.max = window0->fn_req_max;
        }

        FRW(fnreq.min);
        FRW(fnreq.max);

        if (rw == IO_READ) {
            window0->fn_req_min = fnreq.min;
            window0->fn_req_max = fnreq.max;
            wapply_init(window0, tb2->applies.number);
        }
        for (size_t p = 0; p < tb2->applies.number; p++) {
            WApply* wapply = &window0->applies._[p];
            struct {
             uint16_t _05;
             uint16_t _0999;
             uint16_t _099;
             uint16_t _09;
            } dn_bad;
            memset(&dn_bad, 0, sizeof(dn_bad));

            if (rw == IO_WRITE) {
                dn_bad._05 = wapply->dn_bad_05;
                dn_bad._0999 = wapply->dn_bad_0999;
                dn_bad._099 = wapply->dn_bad_099;
                dn_bad._09 = wapply->dn_bad_09;
            }

            FRW(dn_bad);

            if (rw == IO_READ) {
                wapply->dn_bad_05 = dn_bad._05;
                wapply->dn_bad_0999 = dn_bad._0999;
                wapply->dn_bad_099 = dn_bad._099;
                wapply->dn_bad_09 = dn_bad._09;
            }
        
            FRW(wapply->logit);
            FRW(wapply->pset_index);
            FRW(wapply->wcount);
            FRW(wapply->whitelistened);
        }
    }
}

void testbed2_io_windowing(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(tb2->windowing.n.source);
    FRW(tb2->windowing.n.wsize);

    if (rw == IO_READ) {
        INITBY1(tb2->windowing, wsize, TestBed2WindowingBy);
    }

    FORBY(tb2->windowing, wsize) {
        if (rw == IO_READ) {
            INITBY2(tb2->windowing, wsize, source, TestBed2WindowingBy);
        }
    
        FORBY(tb2->windowing, source) {
            RWindowing* windowing_ref;
            SourceIndex source_index;
            WSize wsize;

            windowing_ref = &GETBY2(tb2->windowing, wsize, source);

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

            fflush(file);
        }
    }
}

void testbed2_io_dataset(IOReadWrite rw, FILE* file, RTestBed2 tb2, RDataset0* dataset_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    Index windows_counter;
    size_t idxwsize;
    Index walker;

    if (rw == IO_WRITE) {
        windows_counter = dataset_counter(*dataset_ref);
        idxwsize = (*dataset_ref)->wsize.index;
    }

    FRW(idxwsize);
    FRW(windows_counter);

    if (rw == IO_READ) {
        (*dataset_ref) = dataset0_create(tb2->wsizes._[idxwsize], windows_counter);
    }

    memset(&walker, 0, sizeof(Index));
    for (size_t i = 0; i < windows_counter.all; i++) {
        RWindow0* window_ref = &(*dataset_ref)->windows.all._[i];
        RWindow0* window_bin_ref;
        RWindow0* window_mul_ref;
        RSource source;

        struct {
            uint32_t windowing_idxwindow;
            uint32_t idxsource;
        } windex;
        memset(&windex, 0, sizeof(windex));

        if (rw == IO_WRITE) {
            windex.windowing_idxwindow = (*window_ref)->index;
            windex.idxsource = (*window_ref)->windowing->source->index.all;
        }

        FRW(windex);

        source = tb2->sources._[windex.idxsource];

        window_bin_ref = &(*dataset_ref)->windows.binary[source->wclass.bc]._[walker.binary[source->wclass.bc]++];
        window_mul_ref = &(*dataset_ref)->windows.multi[source->wclass.mc]._[walker.multi[source->wclass.mc]++];

        if (rw == IO_READ) {
            const size_t idxsource = windex.idxsource;
            (*window_ref) = GETBY2(tb2->windowing, wsize, source)->windows._[windex.windowing_idxwindow];
            (*window_bin_ref) = (*window_ref);
            (*window_mul_ref) = (*window_ref);
        }
    }
}

void testbed2_io_all_dataset(IOReadWrite rw, FILE* file, RTestBed2 tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(tb2->dataset.n.try);
    FRW(tb2->dataset.n.wsize);
    FRW(tb2->dataset.n.fold);

    if (rw == IO_READ) {
        INITMANY(tb2->dataset.folds, tb2->dataset.n.fold, FoldConfig);
    }
    for (size_t idxfold = 0; idxfold < tb2->dataset.folds.number; idxfold++) {
        FRW(tb2->dataset.folds._[idxfold].k);
        FRW(tb2->dataset.folds._[idxfold].k_test);
    }
    

    FORBY(tb2->dataset, wsize) {
        FORBY(tb2->dataset, try) {
            testbed2_io_dataset(rw, file, tb2, &GETBY2(tb2->dataset, wsize, try).dataset);
            if (rw == IO_READ) {
                INITBY(tb2->dataset, GETBY2(tb2->dataset, wsize, try), fold, TestBed2DatasetBy);
            }
            FORBY(tb2->dataset, fold) {
                FRW(GETBY3(tb2->dataset, wsize, try, fold).isok);
                // if (0 == GETBY3(tb2->dataset, wsize, try, fold).isok) {
                //     continue;
                // }
                if (rw == IO_READ) {
                    INITMANY(GETBY3(tb2->dataset, wsize, try, fold).splits, tb2->dataset.folds._[idxfold].k, DatasetSplit0);
                }
                for (size_t k = 0; k < GETBY3(tb2->dataset, wsize, try, fold).splits.number; k++) {
                    testbed2_io_dataset(rw, file, tb2, &GETBY3(tb2->dataset, wsize, try, fold).splits._[k].train);
                    testbed2_io_dataset(rw, file, tb2, &GETBY3(tb2->dataset, wsize, try, fold).splits._[k].test);
                }
            }
        }
    }
}

void testbed2_io_create(IOReadWrite rw, FILE* file, RTestBed2* tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    size_t n_tries = 0;
    MANY(WSize) wsizes;
    memset(&wsizes, 0, sizeof(WSize));

    if (rw == IO_WRITE) {
        n_tries = (*tb2)->dataset.n.try;
        CLONEMANY(wsizes, (*tb2)->wsizes);
    }

    FRW(n_tries);
    testbed2_io_wsizes(rw, file, &wsizes);

    if (rw == IO_READ) {
        *tb2 = testbed2_create(wsizes, n_tries);
    }
    FREEMANY(wsizes);
}

int testbed2_io(IOReadWrite rw, char fpath[PATH_MAX], RTestBed2* tb2) {

    FILE* file;
    file = io_openfile(rw, fpath);
    if (file == NULL) {
        LOG_DEBUG("[%s] impossible to open file <%s>.", rw == IO_WRITE ? "Writing" : "Reading", fpath);
        return -1;
    }

    LOG_TRACE("%s from file %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath);

    __io__debug = 1;
    IOLOGPATH(rw, create);
    testbed2_io_create(rw, file, tb2);

    IOLOGPATH(rw, sources);
    testbed2_io_sources(rw, file, (*tb2));

    IOLOGPATH(rw, parameters);
    testbed2_io_parameters(rw, file, (*tb2));

    IOLOGPATH(rw, windowing);
    testbed2_io_windowing(rw, file, (*tb2));

    IOLOGPATH(rw, all_dataset);
    testbed2_io_all_dataset(rw, file, (*tb2));
    __io__debug = 0;

    fclose(file);

    if (rw == IO_WRITE) {
        char fpath2[PATH_MAX];
        sprintf(fpath2, "%s.md", fpath);
        testbed2_md(fpath2, *tb2);
    }

    return 0;
}

void testbed2_md(char fpath[PATH_MAX], const RTestBed2 tb2) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    FILE* file;

    file = fopen(fpath, "w");
    if (file == NULL) {
        LOG_WARN("impossible to open md-file <%s>.", fpath);
        return;
    }

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

    // FPNL(2, "## Foldings");

    // for (size_t f = 0; f < tb2->folds.number; f++) {

    //     FPNL(2, "## Fold %ld", f);
        
    //     fold_md(file, tb2->folds._[f]);
    // }

    fclose(file);

    #undef FP
    #undef FPNL
}