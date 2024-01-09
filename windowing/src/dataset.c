#include "dataset.h"

#include "gatherer.h"
#include "logger.h"
#include "sources.h"
#include "windowing.h"

#include <string.h>

RGatherer datasets_gatherer = NULL;

void dataset_free(void* item) {
    RDataset dataset = (RDataset) item;
    DGAFOR(cl) {
        FREEMANY(dataset->windows.multi[cl]);
    }
    FREEMANY(dataset->windows.binary[0]);
    FREEMANY(dataset->windows.binary[1]);
    FREEMANY(dataset->windows.all);
}

RDataset dataset_alloc() {
    if (datasets_gatherer == NULL) {
        gatherer_alloc(&datasets_gatherer, "datasets", dataset_free, 10000, sizeof(__Dataset), 10);
    }
    return (RDataset) gatherer_alloc_item(datasets_gatherer);;
}

RDataset dataset_create(WSize wsize, Index counter) {
    RDataset dataset;

    dataset = dataset_alloc();

    dataset->wsize = wsize;
    
    if (counter.all)
        MANY_INIT(dataset->windows.all, counter.all, RWindow);
    if (counter.binary[0])
        MANY_INIT(dataset->windows.binary[0], counter.binary[0], RWindow);
    if (counter.binary[1])
        MANY_INIT(dataset->windows.binary[1], counter.binary[1], RWindow);
    DGAFOR(cl) {
        if (counter.multi[cl])
            MANY_INIT(dataset->windows.multi[cl], counter.multi[cl], RWindow);
    }

    return dataset;
}

int dataset_splits_ok(MANY(DatasetSplit) splits) {
    return (splits.number == 0 && splits._ == 0x0) == 0;
}

void dataset_shuffle(RDataset dataset) {
    windows_shuffle(dataset->windows.all);
    windows_shuffle(dataset->windows.binary[0]);
    windows_shuffle(dataset->windows.binary[1]);
    DGAFOR(cl) {
        windows_shuffle(dataset->windows.multi[cl]);
    }
}

DatasetSplits dataset_splits(RDataset dataset, const size_t _k, const size_t _k_test) {
    DatasetSplits splits;

    DGAFOR(cl) {
        const size_t windows_cl_number = dataset->windows.multi[cl].number;
        const size_t kfold_size = windows_cl_number / _k;
        if (cl != 1 && kfold_size == 0) {
            printf("Error: impossible to split the dataset with k=%ld (wn[%d]=%ld, ksize=%ld).\n", _k, cl, windows_cl_number, kfold_size);
            memset(&splits, 0, sizeof(DatasetSplits));
            splits.isok = 0;
            return splits;
        }
    }

    const size_t KFOLDs = _k;
    const size_t TRAIN_KFOLDs = _k - _k_test;
    const size_t TEST_KFOLDs = _k_test;

    splits.isok = 1;

    MANY_INIT(splits.splits, KFOLDs, DatasetSplit);

    {
        Index counter;
        memset(&counter, 0, sizeof(Index));
        for (size_t k = 0; k < KFOLDs; k++) {
            splits.splits._[k].train = dataset_create(dataset->wsize, counter);
            splits.splits._[k].test = dataset_create(dataset->wsize, counter);
        }
    }


    Index train_counter[KFOLDs];
    Index test_counter[KFOLDs];
    memset(&train_counter, 0, KFOLDs * sizeof(Index));
    memset(&test_counter, 0, KFOLDs * sizeof(Index));
    DGAFOR(cl) {
        MANY(RWindow) windows = dataset->windows.multi[cl];

        if (windows.number == 0) {
            LOG_WARN("Dataset-split: empty windows for class %d\n", cl);
            continue;
        }
    
        const size_t kfold_size = windows.number / KFOLDs;
        const size_t kfold_size_rest = windows.number - (kfold_size * KFOLDs);

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

            MANY(RWindow)* train = &splits.splits._[k].train->windows.multi[cl];
            MANY(RWindow)* test = &splits.splits._[k].test->windows.multi[cl];

            MANY_INITREF(train, train_size, RWindow);
            MANY_INITREF(test, test_size, RWindow);

            train_index = 0;
            test_index = 0;
            for (size_t kk = 0; kk < KFOLDs; kk++) {

                size_t start = kk * kfold_size;
                size_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&train->_[train_index], &windows._[start], sizeof(RWindow) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&test->_[test_index], &windows._[start], sizeof(RWindow) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }

    for (size_t k = 0; k < KFOLDs; k++) {
        RDataset train = splits.splits._[k].train;
        RDataset test = splits.splits._[k].test;

        MANY_INITREF(&train->windows.all, train_counter[k].all, RWindow);
        MANY_INITREF(&train->windows.binary[0], train_counter[k].binary[0], RWindow);
        MANY_INITREF(&train->windows.binary[1], train_counter[k].binary[1], RWindow);

        MANY_INITREF(&test->windows.all, test_counter[k].all, RWindow);
        MANY_INITREF(&test->windows.binary[0], test_counter[k].binary[0], RWindow);
        MANY_INITREF(&test->windows.binary[1], test_counter[k].binary[1], RWindow);
    }

    Index train_counter_2[KFOLDs];
    Index test_counter_2[KFOLDs];
    memset(&train_counter_2, 0, KFOLDs * sizeof(Index));
    memset(&test_counter_2, 0, KFOLDs * sizeof(Index));
    DGAFOR(cl) {
        for (size_t k = 0; k < KFOLDs; k++) {
            RDataset train = splits.splits._[k].train;
            RDataset test = splits.splits._[k].test;

            memcpy(&train->windows.all._[train_counter_2[k].all], train->windows.multi[cl]._, sizeof(RWindow) * train->windows.multi[cl].number);
            memcpy(&train->windows.binary[cl > 0]._[train_counter_2[k].binary[cl > 0]], train->windows.multi[cl]._, sizeof(RWindow) * train->windows.multi[cl].number);

            memcpy(&test->windows.all._[test_counter_2[k].all], test->windows.multi[cl]._, sizeof(RWindow) * test->windows.multi[cl].number);
            memcpy(&test->windows.binary[cl > 0]._[test_counter_2[k].binary[cl > 0]], test->windows.multi[cl]._, sizeof(RWindow) * test->windows.multi[cl].number);

            train_counter_2[k].all += train->windows.multi[cl].number;
            test_counter_2[k].all += test->windows.multi[cl].number;

            train_counter_2[k].binary[cl > 0] += train->windows.multi[cl].number;
            test_counter_2[k].binary[cl > 0] += test->windows.multi[cl].number;
        }
    }

    return splits;
}

Index dataset_counter(RDataset ds) {
    Index counter;
    
    counter.all = ds->windows.all.number;
    counter.binary[0] = ds->windows.binary[0].number;
    counter.binary[1] = ds->windows.binary[1].number;
    counter.multi[0] = ds->windows.multi[0].number;
    counter.multi[1] = ds->windows.multi[1].number;
    counter.multi[2] = ds->windows.multi[2].number;
    
    return counter;
}

RDataset dataset_from_windowings(MANY(RWindowing) windowings) {
    RDataset ds;
    Index counter;
    memset(&counter, 0, sizeof(Index));
    for (size_t w = 0; w < windowings.number; w++) {
        RWindowing windowing = windowings._[w];
        counter.all += windowing->windows.number;
        counter.binary[windowing->source->wclass.bc] += windowing->windows.number;
        counter.multi[windowing->source->wclass.mc] += windowing->windows.number;
    }

    ds = dataset_create(windowings._[0]->wsize, counter);

    memset(&counter, 0, sizeof(Index));
    for (size_t w = 0; w < windowings.number; w++) {
        RWindowing windowing = windowings._[w];
        const WClass wc = windowing->source->wclass;
        const size_t nw = windowing->windows.number;

        memcpy(&ds->windows.all._[counter.all], windowing->windows._, nw * sizeof(RWindow));
        memcpy(&ds->windows.binary[wc.bc]._[counter.binary[wc.bc]], windowing->windows._, nw * sizeof(RWindow));
        memcpy(&ds->windows.multi[wc.mc]._[counter.multi[wc.mc]], windowing->windows._, nw * sizeof(RWindow));

        counter.all += nw;
        counter.binary[wc.bc] += nw;
        counter.multi[wc.mc] += nw;
    }

    return ds;
}