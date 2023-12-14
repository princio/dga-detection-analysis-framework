#include "dataset.h"

#include "gatherer.h"
#include "sources.h"
#include "windowing.h"

#include <string.h>

RGatherer datasets_gatherer = NULL;

void dataset0_free(void* item) {
    RDataset0 dataset = (RDataset0) item;
    DGAFOR(cl) {
        FREEMANY(dataset->windows.multi[cl]);
    }
    FREEMANY(dataset->windows.binary[0]);
    FREEMANY(dataset->windows.binary[1]);
    FREEMANY(dataset->windows.all);
}

RDataset0 dataset0_alloc() {
    if (datasets_gatherer == NULL) {
        gatherer_alloc(&datasets_gatherer, "datasets", dataset0_free, 10000, sizeof(__Dataset0), 10);
    }
    return (RDataset0) gatherer_alloc_item(datasets_gatherer);;
}

RDataset0 dataset0_create(WSize wsize, Index counter) {
    RDataset0 dataset;

    dataset = dataset0_alloc();

    dataset->wsize = wsize;
    
    if (counter.all)
        INITMANY(dataset->windows.all, counter.all, RWindow0);
    if (counter.binary[0])
        INITMANY(dataset->windows.binary[0], counter.binary[0], RWindow0);
    if (counter.binary[1])
        INITMANY(dataset->windows.binary[1], counter.binary[1], RWindow0);
    DGAFOR(cl) {
        if (counter.multi[cl])
            INITMANY(dataset->windows.multi[cl], counter.multi[cl], RWindow0);
    }

    return dataset;
}

int dataset0_splits_ok(MANY(DatasetSplit0) splits) {
    return (splits.number == 0 && splits._ == 0x0) == 0;
}

DatasetSplits dataset0_splits(RDataset0 dataset0, const size_t _k, const size_t _k_test) {
    DatasetSplits splits;

    DGAFOR(cl) {
        const size_t windows_cl_number = dataset0->windows.multi[cl].number;
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

    // window0s_shuffle(dataset0->windows.all);
    // window0s_shuffle(dataset0->windows.binary[0]);
    // window0s_shuffle(dataset0->windows.binary[1]);
    // DGAFOR(cl) {
    //     window0s_shuffle(dataset0->windows.multi[cl]);
    // }

    INITMANY(splits.splits, KFOLDs, DatasetSplit0);

    {
        Index counter;
        memset(&counter, 0, sizeof(Index));
        for (size_t k = 0; k < KFOLDs; k++) {
            splits.splits._[k].train = dataset0_create(dataset0->wsize, counter);
            splits.splits._[k].test = dataset0_create(dataset0->wsize, counter);
        }
    }


    Index train_counter[KFOLDs];
    Index test_counter[KFOLDs];
    memset(&train_counter, 0, KFOLDs * sizeof(Index));
    memset(&test_counter, 0, KFOLDs * sizeof(Index));
    DGAFOR(cl) {
        MANY(RWindow0) window0s = dataset0->windows.multi[cl];

        if (window0s.number == 0) {
            printf("Dataset-split: empty windows for class %d\n", cl);
            continue;
        }
    
        const size_t kfold_size = window0s.number / KFOLDs;
        const size_t kfold_size_rest = window0s.number - (kfold_size * KFOLDs);

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

            MANY(RWindow0)* train = &splits.splits._[k].train->windows.multi[cl];
            MANY(RWindow0)* test = &splits.splits._[k].test->windows.multi[cl];

            INITMANYREF(train, train_size, RWindow0);
            INITMANYREF(test, test_size, RWindow0);

            train_index = 0;
            test_index = 0;
            for (size_t kk = 0; kk < KFOLDs; kk++) {

                size_t start = kk * kfold_size;
                size_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&train->_[train_index], &window0s._[start], sizeof(RWindow0) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&test->_[test_index], &window0s._[start], sizeof(RWindow0) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }

    for (size_t k = 0; k < KFOLDs; k++) {
        RDataset0 train = splits.splits._[k].train;
        RDataset0 test = splits.splits._[k].test;

        INITMANYREF(&train->windows.all, train_counter[k].all, RWindow0);
        INITMANYREF(&train->windows.binary[0], train_counter[k].binary[0], RWindow0);
        INITMANYREF(&train->windows.binary[1], train_counter[k].binary[1], RWindow0);

        INITMANYREF(&test->windows.all, test_counter[k].all, RWindow0);
        INITMANYREF(&test->windows.binary[0], test_counter[k].binary[0], RWindow0);
        INITMANYREF(&test->windows.binary[1], test_counter[k].binary[1], RWindow0);
    }

    Index train_counter_2[KFOLDs];
    Index test_counter_2[KFOLDs];
    memset(&train_counter_2, 0, KFOLDs * sizeof(Index));
    memset(&test_counter_2, 0, KFOLDs * sizeof(Index));
    DGAFOR(cl) {
        for (size_t k = 0; k < KFOLDs; k++) {
            RDataset0 train = splits.splits._[k].train;
            RDataset0 test = splits.splits._[k].test;

            memcpy(&train->windows.all._[train_counter_2[k].all], train->windows.multi[cl]._, sizeof(RWindow0) * train->windows.multi[cl].number);
            memcpy(&train->windows.binary[cl > 0]._[train_counter_2[k].binary[cl > 0]], train->windows.multi[cl]._, sizeof(RWindow0) * train->windows.multi[cl].number);

            memcpy(&test->windows.all._[test_counter_2[k].all], test->windows.multi[cl]._, sizeof(RWindow0) * test->windows.multi[cl].number);
            memcpy(&test->windows.binary[cl > 0]._[test_counter_2[k].binary[cl > 0]], test->windows.multi[cl]._, sizeof(RWindow0) * test->windows.multi[cl].number);

            train_counter_2[k].all += train->windows.multi[cl].number;
            test_counter_2[k].all += test->windows.multi[cl].number;

            train_counter_2[k].binary[cl > 0] += train->windows.multi[cl].number;
            test_counter_2[k].binary[cl > 0] += test->windows.multi[cl].number;
        }
    }

    return splits;
}

Index dataset_counter(RDataset0 ds) {
    Index counter;
    
    counter.all = ds->windows.all.number;
    counter.binary[0] = ds->windows.binary[0].number;
    counter.binary[1] = ds->windows.binary[1].number;
    counter.multi[0] = ds->windows.multi[0].number;
    counter.multi[1] = ds->windows.multi[1].number;
    counter.multi[2] = ds->windows.multi[2].number;
    
    return counter;
}

RDataset0 dataset0_from_windowings(MANY(RWindowing) windowings) {
    RDataset0 ds;
    Index counter;
    memset(&counter, 0, sizeof(Index));
    for (size_t w = 0; w < windowings.number; w++) {
        RWindowing windowing = windowings._[w];
        counter.all += windowing->windows.number;
        counter.binary[windowing->source->wclass.bc] += windowing->windows.number;
        counter.multi[windowing->source->wclass.mc] += windowing->windows.number;
    }

    ds = dataset0_create(windowings._[0]->wsize, counter);

    memset(&counter, 0, sizeof(Index));
    for (size_t w = 0; w < windowings.number; w++) {
        RWindowing windowing = windowings._[w];
        const WClass wc = windowing->source->wclass;
        const size_t nw = windowing->windows.number;

        memcpy(&ds->windows.all._[counter.all], windowing->windows._, nw * sizeof(RWindow0));
        memcpy(&ds->windows.binary[wc.bc]._[counter.binary[wc.bc]], windowing->windows._, nw * sizeof(RWindow0));
        memcpy(&ds->windows.multi[wc.mc]._[counter.multi[wc.mc]], windowing->windows._, nw * sizeof(RWindow0));

        counter.all += nw;
        counter.binary[wc.bc] += nw;
        counter.multi[wc.mc] += nw;
    }

    return ds;
}