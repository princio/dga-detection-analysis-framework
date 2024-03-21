
#include "windowmc.h"

#include "gatherer2.h"
#include "windowmany.h"
#include "windowing.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


void _windowmc_free(void* item);
void _windowmc_io(IOReadWrite rw, FILE* file, void**);

G2Config g2_config_wmc = {
    .element_size = sizeof(__WindowMC),
    .size = 0,
    .freefn = _windowmc_free,
    .iofn = _windowmc_io,
    .id = G2_WMC
};

IndexMC windowmc_count(RWindowMC wmc) {
    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));

    counter.all = wmc->all->number;
    BINFOR(bn) counter.binary[bn] = wmc->binary[bn]->number;
    DGAFOR(cl) counter.multi[cl] = wmc->multi[cl]->number;
    
    return counter;
}

RWindowMC windowmc_clone(RWindowMC windowmc) {
    return windowmc_alloc_by_windowmany(windowmc->all);
}

void windowmc_shuffle(RWindowMC windowmc) {
    windowmany_shuffle(windowmc->all);

    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));

    for (size_t w = 0; w < windowmc->all->number; w++) {
        RWindow window = windowmc->all->_[w];
        windowmc->binary[counter.binary[window->windowing->source->wclass.bc]++];
        windowmc->multi[counter.multi[window->windowing->source->wclass.mc]++];
    }
}

void windowmc_minmax(RWindowMC windowmc) {
    assert(windowmc->all->number > 0);
    assert(windowmc->all->_[0]->applies.number);
    
    int64_t logits[windowmc->all->number];

    const size_t n_configs = windowmc->all->_[0]->applies.number;

    MANY_INIT(windowmc->minmax, n_configs, MinMax);

    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        windowmc->minmax._[idxconfig].max = DBL_MIN;
        windowmc->minmax._[idxconfig].min = DBL_MAX;
    }

    for (size_t idxwindow = 0; idxwindow < windowmc->all->number; idxwindow++) {
        for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
            double *min = &windowmc->minmax._[idxconfig].min;
            double *max = &windowmc->minmax._[idxconfig].max;

            const double logit = windowmc->all->_[idxwindow]->applies._[idxconfig].logit;

            if (*min > logit) *min = logit;
            if (*max < logit) *max = logit;
        }
    }
}

void _windowmc_windowingmany_fill(RWindowMC rwindowmc, MANY(RWindowing) windowingmany) {
    if (windowingmany.number == 0) {
        LOG_ERROR("windowing-many elements number is 0.");
        return NULL;
    }

    IndexMC counter = windowingmany_count(windowingmany);

    rwindowmc->all = windowmany_alloc_size(counter.all);
    BINFOR(bn) rwindowmc->binary[bn] = windowmany_alloc_size(counter.binary[bn]);
    DGAFOR(cl) rwindowmc->multi[cl] = windowmany_alloc_size(counter.multi[cl]);
    
    IndexMC index;
    memset(&index, 0, sizeof(IndexMC));
    for (size_t g = 0; g < windowingmany.number; g++) {
        RWindowing windowing = windowingmany._[g];

        for (size_t w = 0; w < windowing->windowmany->number; w++) {
            RWindow window = windowing->windowmany->_[w];
            WClass wc = windowing->source->wclass;
            rwindowmc->all->_[index.all++] = window;
            rwindowmc->binary[wc.bc]->_[index.binary[wc.bc]++] = window;
            rwindowmc->multi[wc.mc]->_[index.binary[wc.mc]++] = window;
        }
    }
}

void _windowmc_windowmany_fill(RWindowMC rwindowmc, RWindowMany windowmany) {
    if (windowmany->number == 0) {
        LOG_ERROR("window-many elements number is 0.");
        return NULL;
    }

    IndexMC counter = windowmany_count(windowmany);

    rwindowmc->all = windowmany_alloc_size(counter.all);
    BINFOR(bn) rwindowmc->binary[bn] = windowmany_alloc_size(counter.binary[bn]);
    DGAFOR(cl) rwindowmc->multi[cl] = windowmany_alloc_size(counter.multi[cl]);
    
    IndexMC index;
    memset(&index, 0, sizeof(IndexMC));
    for (size_t g = 0; g < windowmany->number; g++) {
        RWindowing windowing = windowmany->_[g];

        for (size_t w = 0; w < windowing->windowmany->number; w++) {
            RWindow window = windowing->windowmany->_[w];
            WClass wc = windowing->source->wclass;
            rwindowmc->all->_[index.all++] = window;
            rwindowmc->binary[wc.bc]->_[index.binary[wc.bc]++] = window;
            rwindowmc->multi[wc.mc]->_[index.binary[wc.mc]++] = window;
        }
    }
}

RWindowMC windowmc_alloc() {
    return (RWindowMC) g2_insert_and_alloc(G2_WMC);
}

RWindowMC windowmc_alloc_by_windowmany(RWindowMany windowmany) {
    RWindowMC rwindowmc;
    
    rwindowmc = windowmc_alloc();

    _windowmc_windowmany_fill(rwindowmc, windowmany);

    return rwindowmc;
}

RWindowMC windowmc_alloc_by_windowingmany(MANY(RWindowing) windowingmany) {
    RWindowMC rwindowmc;
    
    rwindowmc = windowmc_alloc();

    _windowmc_windowingmany_fill(rwindowmc, windowingmany);

    return rwindowmc;
}

RWindowMC windowmc_alloc_by_size(IndexMC size) {
    RWindowMC rwindowmc;
    
    rwindowmc = windowmc_alloc();

    rwindowmc->all = windowmany_alloc_size(size.all);
    BINFOR(bc) rwindowmc->binary[bc] = windowmany_alloc_size(size.binary[bc]);
    DGAFOR(mc) rwindowmc->multi[mc] = windowmany_alloc_size(size.multi[mc]);

    return rwindowmc;
}

void _windowmc_free(void* item) {
    RWindowMC rwindowmc = (RWindowMC) item;
}

void _windowmc_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowMC* windowmc = item;

    g2_io_call(G2_WMANY, rw);

    {
        IndexMC indexmc;

        if (IO_IS_WRITE(rw)) {
            indexmc = windowmc_count(*windowmc);
        }

        FRW(indexmc);

        if (IO_IS_READ(rw)) {
            *windowmc = windowmc_alloc(indexmc);
        }
    }

    {
        __MANY many;
        G2Index windowmany_g2index;
        RWindowMany* tmp[6] = {
            &(*windowmc)->all,
            &(*windowmc)->binary[0],
            &(*windowmc)->binary[1],
            &(*windowmc)->multi[0],
            &(*windowmc)->multi[1],
            &(*windowmc)->multi[2]
        };

        if (IO_IS_READ(rw)) {
            many = g2_array("windowmany");
        }

        for (size_t i = 0; i < 6; i++) {
            if (IO_IS_WRITE(rw)) {
                windowmany_g2index = (*tmp[i])->g2index;
            }
            FRW(windowmany_g2index);
            if (IO_IS_READ(rw)) {
                *tmp[i] = many._[windowmany_g2index];
            }
        }
    }
}