
#include "windowing.h"

#include "cache.h"
#include "common.h"
#include "io.h"
#include "parameters.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int io_windowing(IOReadWrite rw, TCPC(Windowing) windowing) {
    char fname[700]; {
        char subdigest[10];

        strncpy(subdigest, windowing->pset->digest, sizeof subdigest);
        subdigest[9] = '\0';
        sprintf(fname, "%s/%s_%s.bin", CACHE_PATH, windowing->source->name, subdigest);
    }

    if(rw == IO_WRITE && io_fileexists(fname)) {
        printf("Warning: the file %s already exists.", fname);
    }

    FILE* file;
    int error;

    error = 0;
    file = io_openfile(rw, fname);
    if (file) {
        FRWNPtr fn = rw ? io_freadN : io_fwriteN;

        if (rw == IO_WRITE) {
            FRW(fn, windowing->windows.number);
        } else {
            int32_t nw = 0;
            FRW(fn, nw);
            if (nw != windowing->windows.number) {
                printf("Warning: nw_read != windows->number [%d != %d]\n", nw, windowing->windows.number);
            }
        }

        for (int i = 0; i < windowing->windows.number; ++i) {
            Window* window = &windowing->windows._[i];

            FRW(fn, window->source_index);
            FRW(fn, window->pset_index);
            
            FRW(fn, window->wnum);

            FRW(fn, window->dgaclass);
            FRW(fn, window->wcount);
            FRW(fn, window->logit);
            FRW(fn, window->whitelistened);
            FRW(fn, window->dn_bad_05);
            FRW(fn, window->dn_bad_09);
            FRW(fn, window->dn_bad_099);
            FRW(fn, window->dn_bad_0999);
        }

        if (fclose(file)) {
            printf("Error %d\n", __LINE__);
        }
    } else {
        error = 1;
    }

    return error;
}

void windowing_windows_init(T_PC(Windowing) windowing) {
    const int32_t nw = N_WINDOWS(windowing->source->fnreq_max, windowing->pset->wsize);
    INITMANY(windowing->windows, nw, Window);
    for (int32_t w = 0; w < nw; w++) {
        windowing->windows._[w].wnum = w;
    }
}

int windowing_load(T_PC(Windowing) windowing) {
    windowing_windows_init(windowing);
    return io_windowing(IO_READ, windowing);
}

int windowing_save(TCPC(Windowing) windowing) {
    return io_windowing(IO_WRITE, windowing);
}

MANY(Windowing) windowing_run_1source_manypsets(TCPC(Source) source, MANY(PSet) psets, WindowingAPFunction fn) {
    MANY(Windowing) windowingaps;
    INITMANY(windowingaps, psets.number, Windowing);

    int32_t n_loaded = 0;
    int32_t loaded[psets.number];
    memset(loaded, 0, psets.number);
    for (int32_t p = 0; p < psets.number; p++) {
        windowingaps._[p].pset = &psets._[p];
        windowingaps._[p].source = source;
        
        int32_t is_loaded = 0 == windowing_load(&windowingaps._[p]);
        loaded[p] = is_loaded;
        n_loaded += is_loaded;
    }

    if (n_loaded < psets.number) {
        fn(source, psets, loaded, windowingaps);
    }

    for (int32_t p = 0; p < psets.number; p++) {
        if (loaded[p]) {
            continue;
        }
        windowing_save(&windowingaps._[p]);
    }

    return windowingaps;
}

void windowing_domainname(const DNSMessage message, TCPC(Windowing) windowing) {
    TCPC(PSet) pset = windowing->pset;
    MANY(Window) const windows = windowing->windows;

    int wnum;
    Window *window;

    wnum = (int) floor(message.fn_req / pset->wsize);

    if (wnum >= windows.number) {
        printf("ERROR\n");
        printf("      wnum: %d\n", wnum);
        printf("     wsize: %d\n", pset->wsize);
        printf("  nwindows: %d\n", windows.number);
    }

    window = &windows._[wnum];

    if (wnum != 0 && wnum != window->wnum) {
        printf("Error[wnum != window->wnum]:\t%d\t%d\n", wnum, window->wnum);
    }

    int whitelistened = 0;
    double value, logit;

    if (pset->windowing == WINDOWING_Q && message.is_response) {
        return;
    } else
    if (pset->windowing == WINDOWING_R && !message.is_response) {
        return;
    }

    logit = message.logit;
    value = message.value;

    if (message.top10m > 0 && message.top10m < pset->whitelisting.rank) {
        value = 0;
        logit = pset->whitelisting.value;
        whitelistened = 1;
    }
    if (logit == INFINITY) {
        logit = pset->infinite_values.pinf;
    } else
        if (logit == (-1 * INFINITY)) {
        logit = pset->infinite_values.ninf;
    }

    ++window->wcount;
    window->dn_bad_05 += value >= 0.5;
    window->dn_bad_09 += value >= 0.9;
    window->dn_bad_099 += value >= 0.99;
    window->dn_bad_0999 += value >= 0.999;

    window->logit += logit;
    window->whitelistened += whitelistened;
}