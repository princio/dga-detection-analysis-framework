
#include "windowmany.h"

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

void _windowmany_free(void* item);
void _windowmany_io(IOReadWrite rw, FILE* file, void**);
void _windowmany_hash(void* item, SHA256_CTX*);

G2Config g2_config_wmany = {
    .element_size = sizeof(__WindowMany),
    .size = 0,
    .freefn = _windowmany_free,
    .iofn = _windowmany_io,
    .hashfn = _windowmany_hash,
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
        counter.multi[wc.mc]++;
    }

    return counter;
}

MinMax windowmany_minmax_config(RWindowMany windowmany, const size_t idxconfig) {
    assert(windowmany->number > 0);

    MinMax minmax;

    minmax.max = -DBL_MAX;
    minmax.min = DBL_MAX;

    for (size_t idxwindow = 0; idxwindow < windowmany->number; idxwindow++) {
        const double logit = windowmany->_[idxwindow]->applies._[idxconfig].logit;

        if (minmax.min > logit) minmax.min = logit;
        if (minmax.max < logit) minmax.max = logit;
    }

    return minmax;
}

MANY(MinMax) windowmany_minmax(RWindowMany windowmany) {
    assert(windowmany->number > 0);
    
    MANY_DECL(minmaxmany, MinMax);
    int64_t logits[windowmany->number];

    const size_t n_configs = windowmany->_[0]->applies.number;

    MANY_INIT(minmaxmany, n_configs, MinMax);

    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        minmaxmany._[idxconfig].max = - DBL_MAX;
        minmaxmany._[idxconfig].min = DBL_MAX;
    }

    for (size_t idxwindow = 0; idxwindow < windowmany->number; idxwindow++) {
        for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
            double *min = &minmaxmany._[idxconfig].min;
            double *max = &minmaxmany._[idxconfig].max;

            const double logit = windowmany->_[idxwindow]->applies._[idxconfig].logit;

            if (*min > logit) *min = logit;
            if (*max < logit) *max = logit;
        }
    }

    return minmaxmany;
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
    assert(windowmany->number == 0);

    MANY_INITREF(windowmany, windows_num, RWindow);
}

void windowmany_buildby_window0many(RWindowMany windowmany, RWindow0Many window0many) {
    windowmany_buildby_size(windowmany, window0many->number);

    for (size_t w = 0; w < window0many->number; w++) {
        windowmany->_[w] = &window0many->_[w];
    }
}

void windowmany_buildby_windowing(RWindowMany windowmany) {
    __MANY many;
    IndexMC counter;

    counter = windowing_count();
    
    windowmany_buildby_size(windowmany, counter.all);

    many = g2_array(G2_WING);
    size_t index = 0;
    for (size_t g = 0; g < many.number; g++) {
        RWindowing windowing = (RWindowing) many._[g];
        for (size_t w = 0; w < windowing->window0many->number; w++) {
            windowmany->_[index] = &windowing->window0many->_[w];
            index++;
        }
    }
}

void _windowmany_free(void* item) {
    RWindowMany windowmany = (RWindowMany) item;
    MANY_FREE((*windowmany));
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

    __MANY many = g2_array(G2_W0MANY);

    for (size_t w = 0; w < (*windowmany)->number; w++) {;
        size_t window0many_index;
        size_t window0many_window_index;
        RWindow* rwindow;

        rwindow = &(*windowmany)->_[w];

        if (IO_IS_WRITE(rw)) {
           window0many_index = (*rwindow)->manyindex;
           window0many_window_index = (*rwindow)->index;
        }

        FRW(window0many_index);
        FRW(window0many_window_index);
    
        RWindow0Many w0m = ((RWindow0Many) many._[window0many_index]);
        assert(w0m->number > window0many_window_index);

        if (IO_IS_READ(rw)) {
            (*rwindow) = &w0m->_[window0many_window_index];
        }
    }
}

void windowmany_hash_update(SHA256_CTX* sha, RWindowMany a) {
    SHA256_Update(sha, &a->g2index, sizeof(G2Index));
    SHA256_Update(sha, &a->number, sizeof(size_t));

    for (size_t w = 0; w < a->number; w++) {
        G2_IO_HASH_UPDATE(a->_[w]->index);
        G2_IO_HASH_UPDATE(a->_[w]->manyindex);
        G2_IO_HASH_UPDATE(a->_[w]->fn_req_min);
        G2_IO_HASH_UPDATE(a->_[w]->fn_req_max);

        G2_IO_HASH_UPDATE_DOUBLE(a->_[w]->duration);
    }
}

void _windowmany_hash(void* item, SHA256_CTX* sha) {
    RWindowMany a = (RWindowMany) item;

    windowmany_hash_update(sha, a);
}