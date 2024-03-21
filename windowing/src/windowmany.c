
#include "windowmany.h"

#include "gatherer2.h"
#include "io.h"
#include "windowing.h"

#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

MAKEMANY(__Window);

void _window0many_free(void* item);
void _window0many_io(IOReadWrite rw, FILE* file, void**);

void _windowmany_free(void* item);
void _windowmany_io(IOReadWrite rw, FILE* file, void**);

G2Config g2_config_w0many = {
    .element_size = sizeof(MANY(__Window)),
    .size = 0,
    .freefn = _window0many_free,
    .iofn = _window0many_io,
    .id = G2_W0MANY
};

G2Config g2_config_wmany = {
    .element_size = sizeof(MANY(RWindow)),
    .size = 0,
    .freefn = _windowmany_free,
    .iofn = _windowmany_io,
    .id = G2_WMANY
};

IndexMC windowmany_count(RWindowMany windowmany) {
    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));

    for (size_t w = 0; w < windowmany->number; w++) {
        RWindow window = windowmany->_[w];
        WClass wc = window->windowing->source->wclass;
        counter.all++;
        counter.binary[wc.bc]++;
        counter.binary[wc.mc]++;
    }

    return counter;
}

void windowmany_shuffle(RWindowMany rwindowmany) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    if (rwindowmany->number > 1) {
        for (size_t i = rwindowmany->number - 1; i > 0; i--) {
            int32_t j = (unsigned int) (drand48() * (i + 1));
            RWindow t = rwindowmany->_[j];
            rwindowmany->_[j] = rwindowmany->_[i];
            rwindowmany->_[i] = t;
        }
    }
}

RWindowMany windowmany_alloc() {
    return (RWindowMany) gatherer_alloc_item(G2_WMANY);
}

RWindowMany windowmany_alloc_size(size_t windows_num) {
    RWindowMany windowmany = windowmany_alloc();
    MANY_INITREF(windowmany, windows_num, RWindow);
    return windowmany;
}

void _window0many_free(void* item) {
    MANY(__Window)* window0many = (MANY(__Window)*) item;

    for (size_t w = 0; w < window0many->number; w++) {
        MANY_FREE(window0many->_[w].applies);
    }

    MANY_FREE((*window0many));
}

RWindowMany window0many_alloc(size_t window_number) {
    MANY(__Window)* window;
    RWindowMany windowmany;

    window = g2_insert(G2_W0MANY);
    MANY_INITREF(window,  window_number, __Window);

    windowmany = windowmany_alloc_size(window_number);

    for (size_t w = 0; w < window->number; w++) {
        window->_[w].index = w;
        window->_[w].fn_req_min = 0;
        window->_[w].fn_req_max = 0;
        window->_[w].windowing = NULL;

        windowmany->_[w] = &windowmany->_[w];
    }

    return windowmany;
}

void _windowmany_free(void* item) {
    RWindowMany windowmany = (RWindowMany) item;
    MANY_FREE((*windowmany));
}

void _window0many_io(IOReadWrite rw, FILE* file, void** item_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowMany* window0many = (MANY(__Window)*) item_ref;

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
        *window0many = window0many_alloc(window0many_number);
    }

    for (size_t w = 0; w < (*window0many)->number; w++) {
        RWindow window;
        size_t idxwindowing;

        window = (*window0many)->_[w];

        FRW(window->index);
        FRW(window->duration);
        FRW(window->fn_req_min);
        FRW(window->fn_req_max);
        
        FRW(window->applies.number);

        __FRW(window->applies._, window->applies.number * sizeof(WApply), file);
    }
}

void _windowmany_io(IOReadWrite rw, FILE* file, void** item_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowMany* windowmany = (RWindowMany*) item_ref;

    __MANY many;
    size_t windowmany_number;

    g2_io_call(G2_W0MANY, rw);

    if (IO_IS_WRITE(rw)) {
        windowmany_number = (*windowmany)->number;
    }

    FRW(windowmany_number);
    
    if (IO_IS_READ(rw)) {
        *windowmany = windowmany_alloc_size(windowmany_number);
        many = g2_array(G2_W0MANY);
    }

    for (size_t w = 0; w < (*windowmany)->number; w++) {;
        size_t windowmany_index;
        size_t windowmany_window_index;
        RWindow window;

        window = (*windowmany)->_[w];

        if (IO_IS_WRITE(rw)) {
           windowmany_index = window->manyindex;
           windowmany_window_index = window->index;
        }

        FRW(windowmany_index);
        FRW(windowmany_window_index);

        if (IO_IS_READ(rw)) {
            RWindowMany windowmany = many._[windowmany_index]; 
            window = windowmany->_[windowmany_window_index];
        }
    }
}