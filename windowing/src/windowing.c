
#include "windowing.h"

#include "gatherer.h"
#include "common.h"
#include "io.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RGatherer windowings_gatherer = NULL;

void _windowing_free(void* item) {
    RWindowing windowing = item;
    FREEMANY(windowing->windows);
}

RWindowing windowings_alloc() {
    if (windowings_gatherer == NULL) {
        gatherer_alloc(&windowings_gatherer, "windowings", _windowing_free, 100, sizeof(__Windowing), 10);
    }
    return (RWindowing) gatherer_alloc_item(windowings_gatherer);
}

RWindowing windowings_create(WSize wsize, RSource source) {
    RWindowing windowing = windowings_alloc();
    const size_t nw = N_WINDOWS(source->fnreq_max, wsize.value);

    windowing->wsize = wsize;
    windowing->source = source;
    windowing->windows = window0s_alloc(nw);

    size_t fnreq = 0;
    for (size_t w = 0; w < nw; w++) {
        windowing->windows._[w]->windowing = windowing;
        windowing->windows._[w]->fn_req_min = fnreq;
        windowing->windows._[w]->fn_req_max = fnreq + wsize.value;
        fnreq += wsize.value;
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
