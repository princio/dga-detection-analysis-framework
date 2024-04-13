#include "windowsplit.h"

#include "detect.h"
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
void _windowsplit_hash(void*, SHA256_CTX*);

G2Config g2_config_wsplit = {
    .element_size = sizeof(__WindowSplit),
    .size = 0,
    .freefn = _windowsplit_free,
    .iofn = _windowsplit_io,
    .hashfn = _windowsplit_hash,
    .id = G2_WSPLIT
};

void windowsplit_shuffle(RWindowSplit split) {
    windowmany_shuffle(split->train);
    windowmc_shuffle(split->test);
}

void windowsplit_init(RWindowSplit windowsplit) {
    windowsplit->train = g2_insert_alloc_item(G2_WMANY);
    windowsplit->test = g2_insert_alloc_item(G2_WMC);
    windowmc_init(windowsplit->test);
}

RWindowSplit windowsplit_createby_day(const int day) {
    __MANY many;
    RWindowSplit split;
    size_t train_counter;
    IndexMC test_counter;

    many = g2_array(G2_WING);

    {
        int count;
        count = 0;
        for (size_t i = 0; i < many.number; i++) {
            count += ((RWindowing) many._[i])->source->day == (int) day;
        }
        if (count == 0) {
            LOG_ERROR("no data available for day %d.", day);
            printf("no data available for day %d.", day);
            exit(1);
        }
    }

    split = (RWindowSplit) g2_insert_alloc_item(G2_WSPLIT);
    windowsplit_init(split);

    { // counting to allocate
        train_counter = 0;
        memset(&test_counter, 0, sizeof(IndexMC));
        for (size_t w = 0; w < many.number; w++) {
            RWindowing windowing = (RWindowing) many._[w];
            if (windowing->source->wclass.bc == BINARYCLASS_0 && windowing->source->day == day) {
                train_counter += windowing->window0many->number;
            } else {
                test_counter.all += windowing->window0many->number;
                test_counter.binary[windowing->source->wclass.bc] += windowing->window0many->number;
                test_counter.multi[windowing->source->wclass.mc] += windowing->window0many->number;
            }
        }

        windowmany_buildby_size(split->train, train_counter);
        windowmc_buildby_size(split->test, test_counter);
    }

    { // setting references
        train_counter = 0;
        memset(&test_counter, 0, sizeof(IndexMC));
        for (size_t g = 0; g < many.number; g++) {
            RWindowing windowing = (RWindowing) many._[g];
            WClass wc = windowing->source->wclass;

            for (size_t w = 0; w < windowing->window0many->number; w++) {
                if (windowing->source->wclass.bc == BINARYCLASS_0 && windowing->source->day == day) {
                    split->train->_[train_counter++] = &windowing->window0many->_[w];
                } else {
                    split->test->all->_[test_counter.all++] = &windowing->window0many->_[w];
                    split->test->binary[wc.bc]->_[test_counter.binary[wc.bc]++] = &windowing->window0many->_[w];
                    split->test->multi[wc.mc]->_[test_counter.multi[wc.mc]++] = &windowing->window0many->_[w];
                }
            }
        }
    }

    return split;
}


RWindowSplit windowsplit_createby_portion(RWindowMC windowmc, const size_t _k, const size_t k_total) {
    assert(_k > 0 && _k <= k_total);

    RWindowSplit split;
    __WindowMany tmp_test; {
        memset(&tmp_test, 0, sizeof(__WindowMany));
    }

    const IndexMC windowmc_counter = windowmc_count(windowmc);
    const size_t k = _k - 1;

    {
        split = (RWindowSplit) g2_insert_alloc_item(G2_WSPLIT);
        windowsplit_init(split);
    }

    const size_t b0_kfold_size = windowmc_counter.binary[0] / k_total;
    const size_t b0_window_index_begin = k * b0_kfold_size;
    const size_t b0_window_index_end = k == k_total ? windowmc_counter.binary[0] : (b0_window_index_begin + b0_kfold_size);

    split->config.how = WINDOWSPLIT_HOW_PORTION;
    split->config.k = k;
    split->config.k_total = k_total;

    {
        windowmany_buildby_size(split->train, b0_kfold_size);
        windowmany_buildby_size(&tmp_test, windowmc_counter.all - b0_kfold_size);
    }

    // printf("[windowsplit] windomc->all->number: %ld\n", windowmc_counter.all);
    // printf("[windowsplit] windomc->bin[0]->number: %ld\n", windowmc_counter.binary[0]);
    // printf("[windowsplit] windomc->bin[1]->number: %ld\n", windowmc_counter.binary[1]);
    // printf("[windowsplit] k/k_total: %ld/%ld\n", _k, k_total);
    // printf("[windowsplit] kfold_size: %ld\n", b0_kfold_size);
    // printf("[windowsplit] window_index_begin: %ld\n", b0_window_index_begin);
    // printf("[windowsplit] window_index_end: %ld\n", b0_window_index_end);
    // printf("[windowsplit] split->train->number: %ld\n", split->train->number);
    // printf("[windowsplit]\n");

    { // setting window references
        size_t index_train_bc0;
        size_t index_train;
        size_t index_test;

        index_train_bc0 = 0;
        index_train = 0;
        index_test = 0;
    
        for (size_t w = 0; w < windowmc->all->number; w++) {
            RWindow window = windowmc->all->_[w];
            WClass wc = window->windowing->source->wclass;

            
            if (wc.bc == BINARYCLASS_0 && index_train_bc0 >= b0_window_index_begin && index_train_bc0 < b0_window_index_end) {
                assert(index_train < split->train->number);
                split->train->_[index_train] = window;
                ++index_train;
            } else {
                assert(index_test < tmp_test.number);
                tmp_test._[index_test] = window;
                ++index_test;
            }
            if (wc.bc == BINARYCLASS_0) {
                index_train_bc0++;
            }
        }
        windowmc_buildby_windowmany(split->test, &tmp_test);
    }

    MANY_FREE(tmp_test);

    return split;
}

void windowsplit_detect(RWindowSplit windowsplit, size_t const idxconfig, Detection* detection) {
    MinMax logitminmax;
    double thzone[N_DETBOUND];
    double th;

    memset(&logitminmax, 0, sizeof(MinMax));
    memset(thzone, 0, sizeof(thzone));

    logitminmax = windowmany_minmax_config(windowsplit->train, idxconfig);
    th = logitminmax.max + 1;

    thzone[0] = - DBL_MAX;
    thzone[N_DETZONE] = DBL_MAX;
    double step = 1.0 / (N_DETBOUND - 2);

    thzone[1] = logitminmax.min;
    thzone[2] = (logitminmax.max + logitminmax.min) / 2;
    thzone[3] = logitminmax.max + 1;

    thzone[4] = thzone[3] + (logitminmax.max - logitminmax.min) * 0.1;

    detect_run(detection, windowsplit->test->all, idxconfig, th, thzone);
}

void _windowsplit_free(void* item) {
    RWindowSplit split = (RWindowSplit) item;
}

void _windowsplit_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowSplit* windowsplit = (RWindowSplit*) item;
    
    G2Index windowmany_g2index_train;
    G2Index windowmc_g2index_test;

    g2_io_call(G2_WMC, rw);

    FRW((*windowsplit)->config);
    FRW((*windowsplit)->wsize);

    g2_io_index(file, rw, G2_WMANY, (void**) &(*windowsplit)->train);
    g2_io_index(file, rw, G2_WMC, (void**) &(*windowsplit)->test);
}

void _windowsplit_hash(void* item, SHA256_CTX* sha) {
    RWindowSplit windowsplit = (RWindowSplit) item;

    G2_IO_HASH_UPDATE(windowsplit->g2index);
    G2_IO_HASH_UPDATE(windowsplit->config);
    G2_IO_HASH_UPDATE(windowsplit->wsize);

    windowmany_hash_update(sha, windowsplit->train);
    windowmc_hash_update(sha, windowsplit->test);
}