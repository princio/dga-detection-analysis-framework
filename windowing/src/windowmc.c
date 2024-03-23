
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


void _windowmc_free(void*);
void _windowmc_io(IOReadWrite, FILE*, void**);
void _windowmc_hash(void*, SHA256_CTX*);

G2Config g2_config_wmc = {
    .element_size = sizeof(__WindowMC),
    .size = 0,
    .freefn = _windowmc_free,
    .iofn = _windowmc_io,
    .hashfn = _windowmc_hash,
    .id = G2_WMC
};

int windowmc_isinit(RWindowMC wmc) {
    int isinit = 1;

    isinit &= wmc->all ? 1 : 0;
    BINFOR(bc) isinit &= wmc->binary[bc] ? 1 : 0;
    DGAFOR(mc) isinit &= wmc->multi[mc] ? 1 : 0;

    return isinit;
}

IndexMC windowmc_count(RWindowMC wmc) {
    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));

    if (!windowmc_isinit(wmc)) {
        windowmc_init(wmc);
        return counter;
    }

    counter.all = wmc->all->number;
    BINFOR(bn) counter.binary[bn] = wmc->binary[bn]->number;
    DGAFOR(cl) counter.multi[cl] = wmc->multi[cl]->number;
    
    return counter;
}

void windowmc_clone(RWindowMC windowmc_src, RWindowMC windowmc_dst) {
    windowmc_buildby_windowmany(windowmc_dst, windowmc_src->all);
}

void windowmc_shuffle(RWindowMC windowmc) {
    assert(windowmc_isinit(windowmc));

    windowmany_shuffle(windowmc->all);

    IndexMC counter;
    memset(&counter, 0, sizeof(IndexMC));

    for (size_t w = 0; w < windowmc->all->number; w++) {
        RWindow window = windowmc->all->_[w];
        WClass wc = window->windowing->source->wclass;

        windowmc->binary[wc.bc]->_[counter.binary[wc.bc]++] = window;
        windowmc->multi[wc.mc]->_[counter.multi[wc.mc]++] = window;
    }
}

void windowmc_minmax(RWindowMC windowmc) {
    assert(windowmc_isinit(windowmc));
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

void windowmc_init(RWindowMC windowmc) {
    assert(!windowmc_isinit(windowmc));

    windowmc->all = g2_insert_alloc_item(G2_WMANY);
    BINFOR(bn) windowmc->binary[bn] = g2_insert_alloc_item(G2_WMANY);
    DGAFOR(cl) windowmc->multi[cl] = g2_insert_alloc_item(G2_WMANY);
}


void windowmc_buildby_size(RWindowMC windowmc, IndexMC size) {
    assert(windowmc_isinit(windowmc));

    windowmany_buildby_size(windowmc->all, size.all);
    BINFOR(bc) windowmany_buildby_size(windowmc->binary[bc], size.binary[bc]);
    DGAFOR(mc) windowmany_buildby_size(windowmc->multi[mc], size.multi[mc]);
}

void windowmc_buildby_windowmany(RWindowMC windowmc, RWindowMany windowmany) {
    assert(windowmc_isinit(windowmc));

    assert(windowmany->number > 0);

    IndexMC index;
    IndexMC counter;
    
    counter = windowmany_count(windowmany);

    windowmc_buildby_size(windowmc, counter);
    
    memset(&index, 0, sizeof(IndexMC));

    for (size_t w = 0; w < windowmany->number; w++) {
        RWindow window  = windowmany->_[w];
        WClass wc = window->windowing->source->wclass;

        windowmc->all->_[index.all++] = window;
        windowmc->binary[wc.bc]->_[index.binary[wc.bc]++] = window;
        windowmc->multi[wc.mc]->_[index.binary[wc.mc]++] = window;
    }
}

void windowmc_buildby_windowing_many(RWindowMC windowmc, MANY(RWindowing) windowingmany) {
    assert(windowmc_isinit(windowmc));

    assert(windowingmany.number > 0);

    IndexMC index;
    IndexMC counter;
    
    counter = windowing_many_count(windowingmany);

    windowmc_buildby_size(windowmc, counter);
    
    memset(&index, 0, sizeof(IndexMC));

    for (size_t g = 0; g < windowingmany.number; g++) {
        RWindowing windowing = windowingmany._[g];

        for (size_t w = 0; w < windowing->windowmany->number; w++) {
            RWindow window = windowing->windowmany->_[w];
            WClass wc = windowing->source->wclass;
            windowmc->all->_[index.all++] = window;
            windowmc->binary[wc.bc]->_[index.binary[wc.bc]++] = window;
            windowmc->multi[wc.mc]->_[index.multi[wc.mc]++] = window;
        }
    }
}

void _windowmc_free(void* item) {
    RWindowMC rwindowmc = (RWindowMC) item;
}

void _windowmc_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowMC* windowmc = (RWindowMC*) item;

    if (IO_IS_WRITE(rw)) {
        assert(windowmc_isinit(*windowmc));
    }

    g2_io_call(G2_WMANY, rw);

    {
        IndexMC indexmc;

        if (IO_IS_WRITE(rw)) {
            indexmc = windowmc_count(*windowmc);
        }

        FRW(indexmc);
    }

    {
        G2Index windowmany_g2index;
        RWindowMany* tmp[6] = {
            &(*windowmc)->all,
            &(*windowmc)->binary[0],
            &(*windowmc)->binary[1],
            &(*windowmc)->multi[0],
            &(*windowmc)->multi[1],
            &(*windowmc)->multi[2]
        };

        for (size_t i = 0; i < 6; i++) {
            g2_io_index(file, rw, G2_WMANY, (void**) tmp[i]);
        }
    }
}

void windowmc_hash_update(SHA256_CTX* sha, RWindowMC a) {
    RWindowMany tmp[6] = {
        a->all,
        a->binary[0],
        a->binary[1],
        a->multi[0],
        a->multi[1],
        a->multi[2]
    };

    G2_IO_HASH_UPDATE(a->g2index);

    for (size_t i = 0; i < 6; i++) {
        windowmany_hash_update(sha, tmp[i]);
    }
}

void _windowmc_hash(void* item, SHA256_CTX* sha) {
    RWindowMC a = (RWindowMC) item;
    windowmc_hash_update(sha, a);
}