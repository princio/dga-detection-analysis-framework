#include "windowsplit.h"

#include "gatherer2.h"
#include "source.h"
#include "windowing.h"
#include "windowmc.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

void _windowsplit_free(void*);
void _windowsplit_io(IOReadWrite, FILE*, void**);

G2Config g2_config_wsplit = {
    .element_size = sizeof(__WindowSplit),
    .size = 0,
    .freefn = _windowsplit_free,
    .iofn = _windowsplit_io,
    .id = G2_WSPLIT
};

RWindowSplit windowsplit_alloc() {
    return (RWindowSplit) g2_insert_and_alloc(G2_WSPLIT);
}

void windowsplit_shuffle(RWindowSplit split) {
    windowmany_shuffle(split->train);
    windowmc_shuffle(split->test);
}

RWindowSplit windowsplit_by_day(MANY(RWindowing) windowingmany, int day) {
    RWindowSplit split;
    size_t train_counter;
    IndexMC test_counter;

    split = windowsplit_alloc();

    #define DGA0_AND_FIRSTDAY windowing->source->wclass.bc == BINARYCLASS_0 && windowing->source->day == 1

    { // counting to allocate
        train_counter = 0;
        memset(&test_counter, 0, sizeof(IndexMC));
        for (size_t w = 0; w < windowingmany.number; w++) {
            RWindowing windowing = windowingmany._[w];
            if (DGA0_AND_FIRSTDAY) {
                train_counter += windowing->windowmany->number;
            } else {
                test_counter.all += windowing->windowmany->number;
                test_counter.binary[windowing->source->wclass.bc] += windowing->windowmany->number;
                test_counter.multi[windowing->source->wclass.mc] += windowing->windowmany->number;
            }
        }

        split->train = windowmany_alloc(train_counter);
        split->test = windowmc_alloc(test_counter);
    }

    { // counting to allocate
        train_counter = 0;
        memset(&test_counter, 0, sizeof(IndexMC));
        for (size_t g = 0; g < windowingmany.number; g++) {
            RWindowing windowing = windowingmany._[g];
            WClass wc = windowing->source->wclass;

            for (size_t w = 0; w < windowing->windowmany->number; w++) {
                if (DGA0_AND_FIRSTDAY) {
                    split->train->_[train_counter++] = windowing->windowmany->_[w];
                } else {
                    split->test->all->_[test_counter.all++] = windowing->windowmany->_[w];
                    split->test->binary[wc.bc]->_[test_counter.binary[wc.bc]++] = windowing->windowmany->_[w];
                    split->test->multi[wc.mc]->_[test_counter.multi[wc.mc]++] = windowing->windowmany->_[w];
                }
            }
        }
    }

    #undef DGA0_AND_FIRSTDAY

    return split;
}

RWindowSplit windowsplit_by_portion(RWindowMC windowmc, int k, int k_total) {
    RWindowSplit split;
    RWindowMany test_windowmany;

    size_t portion_counter;
    IndexMC test_counter;

    const size_t kfold_size = windowmc->binary[BINARYCLASS_0]->number / k_total;
    const size_t kfold_size_rest = windowmc->binary[BINARYCLASS_0]->number - (kfold_size * k_total);
    const size_t train_counter = kfold_size + (k + 1 == k_total) ? kfold_size_rest : 0;
    const size_t window_index_begin = k * kfold_size;
    const size_t window_index_end = window_index_begin + train_counter;

    split = windowsplit_alloc();

    split->config.how = WINDOWSPLIT_HOW_PORTION;
    split->config.k = k;
    split->config.k_total = k_total;

    {
        test_counter = windowmc_count(windowmc);
        test_counter.all -= train_counter;
        test_counter.binary[0] -= train_counter;
        test_counter.multi[0] -= train_counter;
        split->train = windowmany_alloc(train_counter);
        test_windowmany = windowmany_alloc(test_counter);
    }


    {
        size_t index_train;
        size_t index_test;

        index_train = 0;
        index_test = 0;
    
        for (size_t w = 0; w < windowmc->all->number; w++) {
            RWindow window = windowmc->all->_[w];
            WClass wc = window->windowing->source->wclass;

            if (wc.bc == BINARYCLASS_0 &&
                index_train >= window_index_begin && index_train < window_index_end) {
                split->train->_[index_train] = window;
                ++index_train;
            } else {
                test_windowmany->_[index_test] = window;
                ++index_test;
            }
        }
        split->test = windowmc_alloc_by_windowmany(test_windowmany);
    }

    return split;
}

void _windowsplit_free(void* item) {
    RWindowSplit split = (RWindowSplit) item;
    free(split);
}

void _windowsplit_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowSplit* windowsplit = item;
    
    G2Index windowmany_g2index_train;
    G2Index windowmc_g2index_test;

    g2_io_call(G2_WMC, rw);

    if (IO_IS_READ(rw)) {
        *windowsplit = windowsplit_alloc();
    }

    FRW((*windowsplit)->config);

    FRW((*windowsplit)->wsize);
    FRW((*windowsplit)->minmax);

    if (IO_IS_WRITE(rw)) {
        windowmany_g2index_train = (*windowsplit)->train->g2index;
        windowmc_g2index_test = (*windowsplit)->test->g2index;
    }

    FRW(windowmany_g2index_train);
    FRW(windowmc_g2index_test);

    if (IO_IS_READ(rw)) {
        (*windowsplit)->train = g2_get(G2_WMANY, windowmany_g2index_train);
        (*windowsplit)->test = g2_get(G2_WMC, windowmc_g2index_test);
    }
}