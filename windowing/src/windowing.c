
#include "windowing.h"

#include "configsuite.h"
#include "gatherer2.h"
#include "io.h"
#include "windowmany.h"
#include "stratosphere.h"
#include "stratosphere_windowing.h"
// #include "logger.h"

#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _windowing_free(void*);
void _windowing_io(IOReadWrite, FILE*, void**);
void _windowing_print(void*);
void _windowing_hash(void*, SHA256_CTX*);

G2Config g2_config_wing = {
    .element_size = sizeof(__Windowing),
    .size = 0,
    .freefn = _windowing_free,
    .iofn = _windowing_io,
    .printfn = _windowing_print,
    .hashfn = _windowing_hash,
    .id = G2_WING
};

void _windowing_free(void* item) {
}

IndexMC windowing_count() {
    __MANY many;
    IndexMC counter;

    many = g2_array(G2_WING);

    memset(&counter, 0, sizeof(IndexMC));
    for (size_t g = 0; g < many.number; g++) {
        RWindowing windowing = (RWindowing) many._[g];
        WClass wc = windowing->source->wclass;

        counter.all += windowing->window0many->number;
        counter.binary[wc.bc] += windowing->window0many->number;
        counter.multi[wc.mc] += windowing->window0many->number;
    }

    return counter;
}

void windowing_build(RWindowing windowing, RWindow0Many window0many, size_t wsize, RSource source) {
    windowing->wsize = wsize;
    windowing->source = source;

    assert(source->fnreq_max > 0); // source having zero dn should not be processed

    window0many_buildby_size(window0many, N_WINDOWS(source->fnreq_max, wsize));
    
    windowing->window0many = window0many;
    window0many->windowing = windowing;

    size_t fnreq = 0;
    for (size_t w = 0; w < windowing->window0many->number; w++) {
        RWindow window = &windowing->window0many->_[w];
        window->windowing = windowing;
        window->fn_req_min = fnreq;
        window->fn_req_max = fnreq + wsize;

        MANY_INIT(window->applies, configsuite.configs.number, WApply);

        fnreq += wsize;
    }
}

void windowing_apply(WSize wsize) {
    __MANY sourcemany = g2_array(G2_SOURCE);

    assert(sourcemany.number > 0);

    for (size_t i = 0; i < sourcemany.number; i++) {
        RSource source = (RSource) sourcemany._[i];

        RWindowing windowing;
        RWindow0Many window0many;

        windowing = g2_insert_alloc_item(G2_WING);
        window0many = g2_insert_alloc_item(G2_W0MANY);

        windowing_build(windowing, window0many, wsize, source);

        // stratosphere_apply(windowing);
    }

    stratosphere_windowing_apply();
}

void _windowing_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    RWindowing* windowing = (RWindowing*) item;
    RWindow0Many window0many;
    G2Index window0many_g2index;

    g2_io_call(G2_CONFIGSUITE, rw);
    g2_io_call(G2_SOURCE, rw);
    g2_io_call(G2_W0MANY, rw);

    FRW((*windowing)->g2index);
    FRW((*windowing)->wsize);

    g2_io_index(file, rw, G2_SOURCE, (void**) &(*windowing)->source);

    if (IO_IS_WRITE(rw)) {
        window0many_g2index = (*windowing)->g2index;
    }
    FRW(window0many_g2index);

    (*windowing)->window0many = g2_get(G2_W0MANY, window0many_g2index);
    (*windowing)->window0many->windowing = (*windowing);

    for (size_t w = 0; w < (*windowing)->window0many->number; w++) {
        (*windowing)->window0many->_[w].windowing = *windowing;
    }
}

void _windowing_print(void* item) {
    RWindowing windowing = (RWindowing) item;
    printf("%10s: %s\n", "source", windowing->source->name);
    printf("%10s: %ld\n", "wsize", windowing->wsize);
    printf("%10s: %ld\n", "wnum", windowing->window0many->number);
    printf("configsuite 0");
    for (size_t w = 0; w < windowing->window0many->number; w++) {
        printf("%10s %ld\n", "window", w);
        for (size_t c = 0; c < configsuite.configs.number; c++) {
            printf("\t%10s: %f\n", "logit", windowing->window0many->_[w].applies._[c].logit);
        }
    }
}

void _windowing_hash(void* item, SHA256_CTX* sha) {
    RWindowing windowing = (RWindowing) item;

    G2_IO_HASH_UPDATE(windowing->g2index);
    G2_IO_HASH_UPDATE(windowing->source->g2index);
    G2_IO_HASH_UPDATE(windowing->window0many->g2index);
    G2_IO_HASH_UPDATE(windowing->wsize);
}