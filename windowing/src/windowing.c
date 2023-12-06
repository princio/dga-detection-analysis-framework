
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
int windowing_gatherer_initialized = 0;

void _windowing_init(RWindowing windowing) {
    const WSize wsize = windowing->wsize;
    const RSource source = windowing->source;
    const int32_t nw = N_WINDOWS(source->fnreq_max, wsize.value);

    MANY(RWindow0) windows = window0s_alloc(nw);

    windowing->windows = windows;

    uint32_t fnreq = 0;
    for (int32_t w = 0; w < nw; w++) {
        windows._[w]->windowing = windowing;
        windows._[w]->fn_req_min = fnreq;
        windows._[w]->fn_req_max = fnreq + wsize.value;
        fnreq += wsize.value;
    }
}

void _windowings_realloc(MANY(RWindowing)* windowings, size_t index) {
    assert(index <= windowings->number);

    if (windowings->number == index) {
        const int new_number = windowings->number + 50;
    
        if (windowings->number == 0) {
            windowings->_ = calloc(new_number, sizeof(RWindowing));
        } else {
            windowings->_ = realloc(windowings->_, (new_number) * sizeof(RWindowing));
        }

        windowings->number = new_number;

        for (int32_t s = index; s < new_number; s++) {
            windowings->_[s] = NULL;
        }
    }
}

int32_t _windowings_index(TCPC(MANY(RWindowing)) windowings) {
    size_t s;

    for (s = 0; s < windowings->number; s++) {
        if (windowings->_[s] == NULL) break;
    }

    return s;
}

void windowings_add(MANY(RWindowing)* windowings, RWindowing windowing) {
    const int32_t index = _windowings_index(windowings);

    _windowings_realloc(windowings, index);
    
    windowings->_[index] = windowing;
    windowing->index = index;
}

Index windowing_windowscount(MANY(RWindowing) windowings) {
    Index counter;
    memset(&counter, 0, sizeof(Index));
    for (size_t w = 0; w < windowings.number; w++) {
        counter.all += windowings._[w]->windows.number;
        counter.binary[windowings._[w]->source->wclass.bc] += windowings._[w]->windows.number;
        counter.multi[windowings._[w]->source->wclass.mc] += windowings._[w]->windows.number;
    }
    return counter;
}

RWindowing windowings_alloc(RSource source, WSize wsize) {
    RWindowing windowing = calloc(1, sizeof(__Windowing));

    if (windowing_gatherer_initialized == 0) {
        INITMANY(windowing_gatherer, 50, RWindowing);
        windowing_gatherer_initialized = 1;
    }

    windowings_add(&windowing_gatherer, windowing);

    windowing->source = source;
    windowing->wsize = wsize;
    
    _windowing_init(windowing);

    return windowing;
}

void windowings_finalize(MANY(RWindowing)* windowings) {
    const int32_t index = _windowings_index(windowings);
    windowings->_ = realloc(windowings->_, (windowings->number) * sizeof(RWindowing));
}

void windowings_free() {
    for (size_t s = 0; s < windowing_gatherer.number; s++) {
        if (windowing_gatherer._[s] == NULL) break;
        FREEMANY(windowing_gatherer._[s]->windows);
        free(windowing_gatherer._[s]);
    }
    FREEMANY(windowing_gatherer);
    windowing_gatherer_initialized = 0;
}
