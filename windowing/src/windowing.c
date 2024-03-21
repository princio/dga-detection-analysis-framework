
#include "windowing.h"

#include "gatherer2.h"
#include "io.h"
#include "windowmany.h"
// #include "logger.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _windowing_free(void* item);
void _windowing_io(IOReadWrite rw, FILE* file, void**);

G2Config g2_config_wing = {
    .element_size = sizeof(__Windowing),
    .size = 0,
    .freefn = _windowing_free,
    .iofn = _windowing_io,
    .id = G2_WING
};

void _windowing_free(void* item) {
    RWindowing* rwindowing_ref = item;
    free(*rwindowing_ref);
}

IndexMC windowingmany_count(MANY(RWindowing) windowingmany) {
    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));

    for (size_t g = 0; g < windowingmany.number; g++) {
        RWindowing windowing = windowingmany._[g];
        WClass wc = windowing->source->wclass;

        counter.all += windowing->windowmany->number;
        counter.binary[wc.bc] += windowing->windowmany->number;
        counter.multi[wc.mc] += windowing->windowmany->number;
    }

    return counter;
}

RWindowing windowing_alloc() {
    return gatherer_alloc_item(G2_WING);
}

RWindowing windowing_create(size_t wsize, RSource source) {
    RWindowing windowing;
    const size_t nw = N_WINDOWS(source->fnreq_max, wsize);

    windowing = windowing_alloc();

    windowing->wsize = wsize;
    windowing->source = source;
    windowing->windowmany = window_alloc(nw);

    size_t fnreq = 0;
    for (size_t w = 0; w < nw; w++) {
        windowing->windowmany->_[w]->windowing = windowing;
        windowing->windowmany->_[w]->fn_req_min = fnreq;
        windowing->windowmany->_[w]->fn_req_max = fnreq + wsize;
        fnreq += wsize;
    }
    return windowing;
}

IndexMC windowing_many_windowscount(MANY(RWindowing) windowingmany) {
    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));
    for (size_t w = 0; w < windowingmany.number; w++) {
        counter.all += windowingmany._[w]->windowmany->number;
        counter.binary[windowingmany._[w]->source->wclass.bc] += windowingmany._[w]->windowmany->number;
        counter.multi[windowingmany._[w]->source->wclass.mc] += windowingmany._[w]->windowmany->number;
    }
    return counter;
}

void windowing_apply(ConfigSuite suite) {
    __MANY many = g2_array(G2_WING);

    for (size_t i = 0; i < many.number; i++) {
        RWindowing windowing = (RWindowing) many._[i];

        for (size_t w = 0; w < windowing->windowmany->number; w++) {
            RWindow window = windowing->windowmany->_[w];
            MANY_INIT(window->applies, suite.configs.number, WApply);
        }
        
        stratosphere_apply(windowing, suite);
    }
}

void _windowing_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    RWindowing* windowing = (RWindowing*) item;

    g2_io_call(G2_SOURCE, rw);
    g2_io_call(G2_W0MANY, rw);
    g2_io_call(G2_WMANY, rw);

    G2Index source_g2index;
    G2Index windowmany_g2index;

    FRW((*windowing)->g2index);
    FRW((*windowing)->wsize);

    if (IO_IS_WRITE(rw)) {
        source_g2index = (*windowing)->source->g2index;
        windowmany_g2index = (*windowing)->windowmany->g2index;
    }

    FRW(source_g2index);
    FRW(windowmany_g2index);

    (*windowing)->source = g2_get("source", source_g2index);
    (*windowing)->windowmany = g2_get("windowmany", windowmany_g2index);

    for (size_t w = 0; w < (*windowing)->windowmany->number; w++) {
        (*windowing)->windowmany->_[w]->windowing = windowing;
    }
}