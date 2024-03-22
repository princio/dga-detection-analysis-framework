
#include "windowmany.h"

#include "gatherer2.h"
#include "io.h"
#include "windowing.h"

#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void _window0many_free(void* item);
void _window0many_io(IOReadWrite rw, FILE* file, void**);

void _windowmany_free(void* item);
void _windowmany_io(IOReadWrite rw, FILE* file, void**);

G2Config g2_config_w0many = {
    .element_size = sizeof(__Window0Many),
    .size = 0,
    .freefn = _window0many_free,
    .iofn = _window0many_io,
    .id = G2_W0MANY
};

G2Config g2_config_wmany = {
    .element_size = sizeof(__WindowMany),
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

void windowmany_buildby_size(RWindowMany windowmany, size_t windows_num) {
    MANY_INITREF(windowmany, windows_num, RWindow);
}

void window0many_buildby_size(RWindow0Many window0many, const size_t window_number) {
    MANY_INIT(window0many->__windowmany,  window_number, __Window);

    windowmany_buildby_size(&window0many->__windowmany,  window_number);

    for (size_t w = 0; w < window_number; w++) {
        RWindow window = &window0many->__window0many._[w];

        window->index = w;
        window->fn_req_min = 0;
        window->fn_req_max = 0;
        window->windowing = NULL;

        window0many->__windowmany._[w] = window;
    }
}

void _window0many_free(void* item) {
    MANY(__Window)* window0many = (MANY(__Window)*) item;

    for (size_t w = 0; w < window0many->number; w++) {
        MANY_FREE(window0many->_[w].applies);
    }

    MANY_FREE((*window0many));
}

void _windowmany_free(void* item) {
    RWindowMany windowmany = (RWindowMany) item;
    MANY_FREE((*windowmany));
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
        window0many_number = (*window0many)->__window0many.number;
    }

    FRW(window0many_number);

    if (IO_IS_READ(rw)) {
        window0many_buildby_size(*window0many, window0many_number);
    }

    for (size_t w = 0; w < window0many_number; w++) {
        RWindow window;
        size_t idxwindowing;

        window = &(*window0many)->__window0many._[w];

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

    __MANY gatherer_window0many;
    size_t windowmany_number;

    g2_io_call(G2_W0MANY, rw);

    if (IO_IS_WRITE(rw)) {
        windowmany_number = (*windowmany)->number;
    }

    FRW(windowmany_number);

    if (IO_IS_READ(rw)) {
        windowmany_buildby_size(*windowmany, windowmany_number);
        gatherer_window0many = g2_array(G2_W0MANY);
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
            RWindowMany windowmany = gatherer_window0many._[windowmany_index]; 
            window = windowmany->_[windowmany_window_index];
        }
    }
}