
#include "window0many.h"

#include "gatherer2.h"
#include "io.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void _window0many_free(void* item);
void _window0many_io(IOReadWrite rw, FILE* file, void**);
void _window0many_hash(void* item, SHA256_CTX*);

G2Config g2_config_w0many = {
    .element_size = sizeof(__Window0Many),
    .size = 0,
    .freefn = _window0many_free,
    .iofn = _window0many_io,
    .hashfn = _window0many_hash,
    .id = G2_W0MANY
};

void window0many_buildby_size(RWindow0Many window0many, const size_t window_number) {
    MANY_INITREF(window0many,  window_number, __Window);

    LOG_TRACE("[window0many] buildby size: %ld", window_number);

    for (size_t w = 0; w < window_number; w++) {
        RWindow window = &window0many->_[w];

        window->index = w;
        window->manyindex = window0many->g2index;
        window->fn_req_min = 0;
        window->fn_req_max = 0;
        window->windowing = NULL;

        // window0many->__windowmany._[w] = window;
    }
}

void _window0many_free(void* item) {
    RWindow0Many window0many = (RWindow0Many) item;

    for (size_t w = 0; w < window0many->number; w++) {
        MANY_FREE(window0many->_[w].applies);
    }

    MANY_FREEREF(window0many);
}

void _window0many_io(IOReadWrite rw, FILE* file, void** item_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindow0Many* window0many = (RWindow0Many*) item_ref;

    // we don't put these because we would enter in a forever loop.
    // many to many relationships are handled in source and windowing
    // g2_io_call(G2_SOURCE, rw);
    // g2_io_call(G2_WING, rw);

    size_t window0many_number;

    if (IO_IS_WRITE(rw)) {
        window0many_number = (*window0many)->number;
    }

    FRW(window0many_number);

    if (IO_IS_READ(rw)) {
        window0many_buildby_size(*window0many, window0many_number);
    }

    for (size_t w = 0; w < window0many_number; w++) {
        RWindow window;
        size_t apply_count;

        window = &(*window0many)->_[w];

        FRW(window->index);
        FRW(window->duration);
        FRW(window->fn_req_min);
        FRW(window->fn_req_max);
        FRW(window->n_message);
        
        FRW(window->applies.number);

        if (IO_IS_READ(rw)) {
            MANY_INIT(window->applies, window->applies.number, WApply);
        }

        __FRW(window->applies._, window->applies.number * sizeof(WApply), file);
    }
}

void _window0many_hash(void* item, SHA256_CTX* sha) {
    RWindow0Many a = (RWindow0Many) item;

    G2_IO_HASH_UPDATE(a->g2index);
    G2_IO_HASH_UPDATE(a->number);

    for (size_t w = 0; w < a->number; w++) {
        RWindow window = &a->_[w];
        G2_IO_HASH_UPDATE(window->index);
        G2_IO_HASH_UPDATE(window->manyindex);
        G2_IO_HASH_UPDATE(window->fn_req_min);
        G2_IO_HASH_UPDATE(window->fn_req_max);
        G2_IO_HASH_UPDATE(window->n_message);
        G2_IO_HASH_UPDATE_DOUBLE(window->duration);

        for (size_t c = 0; c < configsuite.configs.number; c++) {
            G2_IO_HASH_UPDATE(window->applies._[c].dn_bad);
            G2_IO_HASH_UPDATE(window->applies._[c].wcount);
            G2_IO_HASH_UPDATE(window->applies._[c].whitelistened);
            G2_IO_HASH_UPDATE_DOUBLE(window->applies._[c].logit);
        }
    }
}