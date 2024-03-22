
#include "windowing.h"

#include "configsuite.h"
#include "gatherer2.h"
#include "io.h"
#include "windowmany.h"
#include "stratosphere.h"
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

IndexMC windowing_many_count(MANY(RWindowing) windowingmany) {
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

void windowing_build(RWindowing windowing, RWindow0Many window0many, size_t wsize, RSource source) {
    windowing->wsize = wsize;
    windowing->source = source;

    window0many_buildby_size(window0many, N_WINDOWS(source->fnreq_max, wsize));
    
    windowing->windowmany = &window0many->__windowmany;

    size_t fnreq = 0;
    for (size_t w = 0; w < windowing->windowmany->number; w++) {
        RWindow window = windowing->windowmany->_[w];
        window->windowing = windowing;
        window->fn_req_min = fnreq;
        window->fn_req_max = fnreq + wsize;

        MANY_INIT(window->applies, configsuite.configs.number, WApply);

        fnreq += wsize;
    }
}

void windowing_apply(WSize wsize) {
    __MANY sourcemany = g2_array(G2_SOURCE);

    for (size_t i = 0; i < sourcemany.number; i++) {
        RSource source = (RSource) sourcemany._[i];

        RWindowing windowing;
        RWindow0Many window0many;

        windowing = g2_insert_alloc_item(G2_WING);
        window0many = g2_insert_alloc_item(G2_W0MANY);

        windowing_build(windowing, window0many, wsize, source);

        stratosphere_apply(windowing);
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
    __Window0Many* window0many;

    FRW((*windowing)->g2index);
    FRW((*windowing)->wsize);

    g2_io_index(file, rw, G2_SOURCE, (void**) &(*windowing)->source);
    g2_io_index(file, rw, G2_SOURCE, (void**) &window0many);

    for (size_t w = 0; w < (*windowing)->windowmany->number; w++) {
        (*windowing)->windowmany->_[w]->windowing = *windowing;
    }
}