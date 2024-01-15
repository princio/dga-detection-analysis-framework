
#include "windowing.h"

#include "gatherer.h"
#include "io.h"
// #include "logger.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RGatherer windowings_gatherer = NULL;

void _windowing_free(void* item) {
    RWindowing* rwindowing_ref = item;
    FREEMANY((*rwindowing_ref)->windows);
    free(*rwindowing_ref);
}

RWindowing windowings_alloc() {
    if (windowings_gatherer == NULL) {
        LOG_DEBUG("Gatherer: creating windowings.");
        gatherer_alloc(&windowings_gatherer, "windowings", _windowing_free, 100, sizeof(RWindowing*), 10);
    }

    RWindowing rwindowing = calloc(1, sizeof(__Windowing));

    RWindowing* rwindowing_ref = (RWindowing*) gatherer_alloc_item(windowings_gatherer);

    *rwindowing_ref  = rwindowing;
    
    return rwindowing;
}

RWindowing windowings_create(size_t wsize, RSource source) {
    RWindowing windowing;
    const size_t nw = N_WINDOWS(source->fnreq_max, wsize);

    windowing = windowings_alloc();

    windowing->wsize = wsize;
    windowing->source = source;
    windowing->windows = windows_alloc(nw);

    size_t fnreq = 0;
    for (size_t w = 0; w < nw; w++) {
        windowing->windows._[w]->windowing = windowing;
        windowing->windows._[w]->fn_req_min = fnreq;
        windowing->windows._[w]->fn_req_max = fnreq + wsize;
        fnreq += wsize;
    }
    return windowing;
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
