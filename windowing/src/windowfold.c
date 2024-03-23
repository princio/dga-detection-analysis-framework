#include "windowfold.h"

#include "gatherer2.h"
#include "source.h"
#include "windowing.h"
#include "windowmc.h"

#include <float.h>
#include <math.h>
#include <string.h>

void _windowfold_free(void*);
void _windowfold_io(IOReadWrite, FILE*, void**);
void _windowfold_hash(void*, SHA256_CTX*);

G2Config g2_config_wfold = {
    .element_size = sizeof(__WindowFold),
    .size = 0,
    .freefn = _windowfold_free,
    .iofn = _windowfold_io,
    .hashfn = _windowfold_hash,
    .id = G2_WFOLD
};

void _windowfold_free(void* item) {
    RWindowFold windowfold = (RWindowFold) item;
    MANY_FREE(windowfold->foldkmany);
}

int windowfold_foldk_ok(RWindowFold windwofold, const size_t k) {
    return (windwofold->foldkmany.number == 0 && windwofold->foldkmany._ == 0x0) == 0;
}

void windowfold_init(RWindowFold windowfold, const WindowFoldConfig config) {
    windowfold->config = config;
    MANY_INIT(windowfold->foldkmany, config.k, WindowFoldK);
    for (size_t k = 0; k < config.k; k++) {
        windowfold->foldkmany._[k].train = g2_insert_alloc_item(G2_WMC);
        windowfold->foldkmany._[k].test = g2_insert_alloc_item(G2_WMC);
        windowmc_init(windowfold->foldkmany._[k].train);
        windowmc_init(windowfold->foldkmany._[k].test);
    }
}

RWindowFold windowfold_create(RWindowMC windowmc, const WindowFoldConfig config) {
    RWindowFold windowfold;

    DGAFOR(cl) {
        const size_t windows_cl_number = windowmc->multi[cl]->number;
        const size_t kfold_size = windows_cl_number / config.k;
        if (cl != 1 && kfold_size == 0) {
            LOG_ERROR("Impossible to split the windowfold with k=%ld and k_test=%ld (wn[%d]=%ld, ksize=%ld).", config.k, config.k_test,  cl, windows_cl_number, kfold_size);
            return NULL;
        }
    }

    windowfold = g2_insert_alloc_item(G2_WFOLD);
    windowfold_init(windowfold, config);

    const size_t KFOLDs = config.k;
    const size_t TRAIN_KFOLDs = config.k - config.k_test;
    const size_t TEST_KFOLDs = config.k_test;

    windowfold->isok = 1;

    IndexMC train_counter[KFOLDs];
    IndexMC test_counter[KFOLDs];
    memset(&train_counter, 0, KFOLDs * sizeof(IndexMC));
    memset(&test_counter, 0, KFOLDs * sizeof(IndexMC));
    DGAFOR(cl) {
        RWindowMany multi = windowmc->multi[cl];

        if (multi->number == 0) {
            LOG_WARN("empty windows for class %d\n", cl);
            continue;
        }
    
        const size_t kfold_size = multi->number / KFOLDs;
        const size_t kfold_size_rest = multi->number - (kfold_size * KFOLDs);

        size_t kindexes[KFOLDs][KFOLDs]; {
            for (size_t k = 0; k < KFOLDs; k++) {
                for (size_t kindex = k; kindex < (k + TRAIN_KFOLDs); kindex++) {
                    kindexes[k][kindex % KFOLDs] = 0;
                }
                for (size_t kindex = k + TRAIN_KFOLDs; kindex < (k + KFOLDs); kindex++) {
                    kindexes[k][kindex % KFOLDs] = 1;
                }
            }
        }
    
        for (size_t k = 0; k < KFOLDs; k++) {
            
            size_t train_index;
            size_t test_index;

            const size_t train_size = kfold_size * TRAIN_KFOLDs + (!kindexes[k][KFOLDs - 1] ? kfold_size_rest : 0);
            const size_t test_size = kfold_size * TEST_KFOLDs + (kindexes[k][KFOLDs - 1] ? kfold_size_rest : 0);

            train_counter[k].all += train_size;
            train_counter[k].binary[cl > 0] += train_size;

            test_counter[k].all += test_size;
            test_counter[k].binary[cl > 0] += test_size;

            RWindowMany train = windowfold->foldkmany._[k].train->multi[cl];
            RWindowMany test = windowfold->foldkmany._[k].test->multi[cl];

            MANY_INITREF(train, train_size, RWindow);
            MANY_INITREF(test, test_size, RWindow);

            train_index = 0;
            test_index = 0;
            for (size_t kk = 0; kk < KFOLDs; kk++) {

                size_t start = kk * kfold_size;
                size_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&train->_[train_index], &multi->_[start], sizeof(RWindow) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&test->_[test_index], &multi->_[start], sizeof(RWindow) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }

    for (size_t k = 0; k < KFOLDs; k++) {
        RWindowMC train = windowfold->foldkmany._[k].train;
        RWindowMC test = windowfold->foldkmany._[k].test;

        windowmany_buildby_size(train->all, train_counter[k].all);
        windowmany_buildby_size(train->binary[0], train_counter[k].binary[0]);
        windowmany_buildby_size(train->binary[1], train_counter[k].binary[1]);

        windowmany_buildby_size(test->all, test_counter[k].all);
        windowmany_buildby_size(test->binary[0], test_counter[k].binary[0]);
        windowmany_buildby_size(test->binary[1], test_counter[k].binary[1]);
    }

    IndexMC train_counter_2[KFOLDs];
    IndexMC test_counter_2[KFOLDs];
    memset(&train_counter_2, 0, KFOLDs * sizeof(IndexMC));
    memset(&test_counter_2, 0, KFOLDs * sizeof(IndexMC));
    DGAFOR(cl) {
        for (size_t k = 0; k < KFOLDs; k++) {
            RWindowMC train = windowfold->foldkmany._[k].train;
            RWindowMC test = windowfold->foldkmany._[k].test;

            memcpy(&train->all->_[train_counter_2[k].all], train->multi[cl]->_, sizeof(RWindow) * train->multi[cl]->number);
            memcpy(&train->binary[cl > 0]->_[train_counter_2[k].binary[cl > 0]], train->multi[cl]->_, sizeof(RWindow) * train->multi[cl]->number);

            memcpy(&test->all->_[test_counter_2[k].all], test->multi[cl]->_, sizeof(RWindow) * test->multi[cl]->number);
            memcpy(&test->binary[cl > 0]->_[test_counter_2[k].binary[cl > 0]], test->multi[cl]->_, sizeof(RWindow) * test->multi[cl]->number);

            train_counter_2[k].all += train->multi[cl]->number;
            test_counter_2[k].all += test->multi[cl]->number;

            train_counter_2[k].binary[cl > 0] += train->multi[cl]->number;
            test_counter_2[k].binary[cl > 0] += test->multi[cl]->number;
        }
    }

    return windowfold;
}

void _windowfold_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    RWindowFold* windowfold = (RWindowFold*) item;

    g2_io_call(G2_WMC, rw);

    FRW((*windowfold)->isok);
    FRW((*windowfold)->config);

    if (IO_IS_READ(rw)) {
        MANY_INIT((*windowfold)->foldkmany, (*windowfold)->config.k, WindowFoldK);
    }

    for (size_t i = 0; i < (*windowfold)->foldkmany.number; i++) {
        g2_io_index(file, rw, G2_WMC, (void**) &(*windowfold)->foldkmany._[i].train);
        g2_io_index(file, rw, G2_WMC, (void**) &(*windowfold)->foldkmany._[i].test);
    }
}

void _windowfold_hash(void* item, SHA256_CTX* sha) {
    RWindowFold windowfold = (RWindowFold) item;

    // SHA256_Update(&sha, &windowfold->g2index, sizeof(G2Index));
    G2_IO_HASH_UPDATE(windowfold->config);

    // for (size_t i = 0; i < windowfold->foldkmany.number; i++) {
    //     windowmc_hash_update(&sha, windowfold->foldkmany._[i].train);
    //     windowmc_hash_update(&sha, windowfold->foldkmany._[i].test);
    // }
}