
#include "windowing.h"

#include "cache.h"
#include "common.h"
#include "io.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void windowing_io(IOReadWrite rw, FILE* file, void* obj) {
    Windowing* windowing = obj;

    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    if (rw == IO_WRITE) {
        FRW(windowing->windows.number);
    } else {
        int32_t nw = 0;
        FRW(nw);
        if (nw != windowing->windows.number) {
            printf("Warning: nw_read != windows->number [%d != %d]\n", nw, windowing->windows.number);
        }
    }

    for (int i = 0; i < windowing->windows.number; ++i) {
        Window* window = &windowing->windows._[i];

        FRW(window->pset_index);
        
        FRW(window->wnum);

        FRW(window->dgaclass);
        FRW(window->wcount);
        FRW(window->logit);
        FRW(window->whitelistened);
        FRW(window->dn_bad_05);
        FRW(window->dn_bad_09);
        FRW(window->dn_bad_099);
        FRW(window->dn_bad_0999);
    }
}

void windowing_init(TCPC(Source) source, TCPC(PSet) pset, T_PC(Windowing) windowing) {
    windowing->source = source;
    windowing->pset = pset;

    const int32_t nw = N_WINDOWS(windowing->source->fnreq_max, windowing->pset->wsize);
    INITMANY(windowing->windows, nw, Window);
    for (int32_t w = 0; w < nw; w++) {
        windowing->windows._[w].pset_index = windowing->pset->id;
        windowing->windows._[w].wnum = w;
    }
}

int windowing_load(T_PC(Windowing) windowing) {
    if (windowing->pset == NULL || windowing->pset == NULL || windowing->windows.number == 0) {
        fprintf(stderr, "Windowing not initialized");
        return -2;
    }
    char objid[IO_OBJECTID_LENGTH];
    windowing_io_objid(windowing, objid);
    return io_load(objid, windowing_io, windowing);
}

int windowing_save(TCPC(Windowing) windowing) {
    return io_save(windowing, windowing_io_objid, windowing_io);
}

MANY(Windowing) windowing_run_1source_manypsets(TCPC(Source) source, MANY(PSet) psets, WindowingAPFunction fn) {
    MANY(Windowing) windowingaps;
    INITMANY(windowingaps, psets.number, Windowing);

    int32_t n_loaded = 0;
    int32_t loaded[psets.number];
    memset(loaded, 0, psets.number);
    for (int32_t p = 0; p < psets.number; p++) {
        windowing_init(source, &psets._[p], &windowingaps._[p]);

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

void windowing_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    TCPC(Windowing) windowing = obj;

    memset(objid, 0, IO_OBJECTID_LENGTH);

    char subdigest_source[IO_SUBDIGEST_LENGTH];
    char subdigest_pset[IO_SUBDIGEST_LENGTH];

    io_subdigest(windowing->pset, sizeof(PSet), subdigest_source);
    io_subdigest(windowing->source, sizeof(Source), subdigest_pset);

    sprintf(objid, "windowing_%s_%s", subdigest_pset, subdigest_source);
}