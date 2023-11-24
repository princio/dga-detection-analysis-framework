
#include "windowing.h"

#include "cache.h"
#include "common.h"
#include "io.h"
#include "windows.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MANY(RWindowing) windowing_gatherer = {
    .number = 0,
    ._ = NULL
};

void _windowings_realloc(MANY(RWindowing)* windowings, int32_t index) {
    assert(index <= windowings->number);

    if (windowing_gatherer.number == index) {
        const int new_number = windowing_gatherer.number + 50;
    
        windowing_gatherer._ = realloc(windowing_gatherer._, (new_number) * sizeof(RWindowing));

        for (int32_t s = index; s < new_number; s++) {
            windowing_gatherer._[s] = NULL;
        }
    }
}

int32_t _windowings_index(TCPC(MANY(RWindowing)) windowings) {
    int32_t s;

    for (s = 0; s < windowings->number; s++) {
        if (windowings->_[s] == NULL) break;
    }

    return s;
}

int32_t windowings_add(MANY(RWindowing)* windowings, RWindowing windowing) {
    const int32_t index = _windowings_index(windowings);

    _windowings_realloc(windowings, index);

    windowings->_[index] = windowing;
    
    return index;
}

RWindowing windowings_alloc(RSource rsource, WSize wsize) {
    RWindowing windowing = calloc(1, sizeof(__Windowing));

    const int32_t index = windowings_add(&windowing_gatherer, windowing);

    windowing->index = index;
    
    windowing_gatherer._[index] = windowing;

    return windowing;
}

void windowings_finalize(MANY(RWindowing)* windowings) {
    const int32_t index = _windowings_index(windowings);
    windowings->_ = realloc(windowings->_, (windowings->number) * sizeof(RWindowing));
}

void windowings_free() {
    for (int32_t s = 0; s < windowing_gatherer.number; s++) {
        if (windowing_gatherer._[s] == NULL) break;
        free(windowing_gatherer._[s]);
    }
    free(windowing_gatherer._);
}

void windowing_run(RWindowing windowing) {
    const WSize wsize = windowing->wsize;
    const RSource source = windowing->source;
    MANY(RWindow0)* windows = &windowing->windows;

    const int32_t nw = N_WINDOWS(source->fnreq_max, wsize);
    INITMANYREF(windows, nw, RWindow0);

    uint32_t fnreq = 0;
    for (int32_t w = 0; w < nw; w++) {
        windows->_[w]->windowing = windowing;
        windows->_[w]->wnum = w;
        windows->_[w]->fn_req_min = fnreq;
        fnreq += wsize;
        windows->_[w]->fn_req_max = fnreq;
    }
}

/*
void windowing_init(TCPC(Windowing) windowing, TCPC(PSet) pset, T_PC(WindowingApply) windowingapply) {
    windowingapply->windowing = windowing;
    windowingapply->pset = pset;

    const int32_t nw = N_WINDOWS(windowingapply->windowing->fnreq_max, windowingapply->pset->wsize);
    windowingapply->windows = window_alloc(nw);
    for (int32_t w = 0; w < nw; w++) {
        windowingapply->windows._[w]->pset_index = windowingapply->pset->id;
        windowingapply->windows._[w]->wnum = w;
    }
}

int windowing_load(T_PC(WindowingApply) windowingapply) {
    if (windowingapply->windowing == NULL || windowingapply->pset == NULL || windowingapply->windows.number == 0) {
        fprintf(stderr, "WindowingApply not initialized");
        return -2;
    }
    char objid[IO_OBJECTID_LENGTH];
    windowing_io_objid(windowingapply, objid);
    return io_load(objid, 1, windowing_io, windowingapply);
}

int windowing_save(TCPC(WindowingApply) windowingapply) {
    return io_save(windowingapply, 1, windowing_io_objid, windowing_io);
}

MANY(WindowingApply) windowing_run_windowing(TCPC(ApplyWindowing) as) {
    TCPC(Windowing) windowing = as->windowing;
    MANY(WindowingApply) sa;
    MANY(ApplyPSet) aps;

    INITMANY(sa, as->applies.number, WindowingApply);
    INITMANY(aps, as->applies.number, ApplyPSet);

    int32_t n_loaded = 0;
    for (int32_t p = 0; p < as->applies.number; p++) {
        ApplyPSet* ap = &as->applies._[p];
        windowing_init(windowing, ap->pset, &sa._[p]);

        if(0 == windowing_load(&sa._[p])) {
            ap->loaded = 1;
            ++n_loaded;
        }

        aps._[p] = *ap;
    }

    if (n_loaded < as->applies.number) {
        (*as->fn)(windowing, aps, sa);
    }

    for (int32_t p = 0; p < as->applies.number; p++) {
        if (as->applies._[p].loaded) {
            continue;
        }
        windowing_save(&sa._[p]);
    }

    return sa;
}

MANY(Windowing0) windowing0_run(TCPC(Windowing) windowing, MANY(WSize) wsizes) {
    MANY(Windowing0) windowingaps;
    INITMANY(windowingaps, wsizes.number, Windowing);

    for (int32_t p = 0; p < wsizes.number; p++) {
        const WSize wsize = wsizes._[p];
        Windowing0* windowing0 = &windowingaps._[p];
    
        windowing0->windowing = windowing;
        windowing0->wsize = wsize;

        const int32_t nw = N_WINDOWS(windowing->fnreq_max, wsize);
        INITMANY(windowing0->windows, nw, Window0);

        uint32_t fnreq = 0;
        for (int32_t w = 0; w < nw; w++) {
            windowing0->windows._[w].windowing = (Windowing*) windowing0->windowing;
            windowing0->windows._[w].wnum = w;
            windowing0->windows._[w].fn_req_min = fnreq;
            windowing0->windows._[w].fn_req_max = fnreq + wsize;
            fnreq += wsize;
        }
    }

    return windowingaps;
}

void windowing_io(IOReadWrite rw, FILE* file, void* obj) {
    WindowingApply* windowing = obj;

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
        RWindow window = windowing->windows._[i];

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

void windowing_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    TCPC(WindowingApply) windowing = obj;

    memset(objid, 0, IO_OBJECTID_LENGTH);

    char subdigest_windowing[IO_SUBDIGEST_LENGTH];
    char subdigest_pset[IO_SUBDIGEST_LENGTH];

    io_subdigest(windowing->pset, sizeof(PSet), subdigest_windowing);
    io_subdigest(windowing->windowing, sizeof(Windowing), subdigest_pset);

    sprintf(objid, "windowing_%s_%d_%s", subdigest_windowing, windowing->wsize, subdigest_pset);
}
*/