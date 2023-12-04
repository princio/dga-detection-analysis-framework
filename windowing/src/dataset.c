#include "dataset.h"

#include "sources.h"
#include "windowing.h"

#include <string.h>

List datasets_gatherer = {.root = NULL};

MAKEMANY(__Dataset0);

void dataset0_init(RDataset0 dataset, Index counter) {
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
}

RDataset0 dataset0_alloc(const Dataset0Init init) {
    __Dataset0 dataset;
    memset(&dataset, 0, sizeof(__Dataset0));

    dataset0_init(&dataset, init.windows_counter);
    dataset.applies_number = init.applies_number;
    dataset.wsize = init.wsize;

    if (datasets_gatherer.root == NULL) {
        list_init(&datasets_gatherer, sizeof(__Dataset0));
    }

    ListItem* item = list_insert_copy(&datasets_gatherer, &dataset);

    RDataset0 rds = item->item;

    return rds;
}

MANY(DatasetSplit0) dataset0_split_k(RDataset0 dataset0, const size_t k, const size_t k_test) {
    MANY(DatasetSplit0) splits;
    memset(&splits, 0, sizeof(MANY(DatasetSplit0)));

    DGAFOR(cl) {
        const size_t windows_cl_number = dataset0->windows.multi[cl].number;
        const size_t kfold_size = windows_cl_number / k;
        if (kfold_size == 0) {
            printf("Error: impossible to split the dataset with k=%ld.\n", k);
        }
        return splits;
    }

    
    const size_t KFOLDs = k;
    const size_t TRAIN_KFOLDs = k - k_test;
    const size_t TEST_KFOLDs = k_test;

    INITMANY(splits, k, DatasetSplit0);

    {
        Index counter;
        memset(&counter, 0, sizeof(Index));

        Dataset0Init init = {
            .applies_number = dataset0->applies_number,
            .wsize = dataset0->wsize,
        };
        for (size_t k = 0; k < KFOLDs; k++) {
            splits._[k].train = dataset0_alloc(init);
            splits._[k].test = dataset0_alloc(init);
        }
    }

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

            MANY(RWindow0)* train = &splits._[k].train->windows.multi[cl];
            MANY(RWindow0)* test = &splits._[k].test->windows.multi[cl];

            printf("--- %ld\t%ld\t%ld\t", kfold_size, TRAIN_KFOLDs, TEST_KFOLDs);
            printf("--- %ld\t%ld\n", train_size, test_size);
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

    return splits;
}

void dataset0_free(RDataset0 dataset) {
    DGAFOR(cl) {
        FREEMANY(dataset->windows.multi[cl]);
    }
    FREEMANY(dataset->windows.binary[0]);
    FREEMANY(dataset->windows.binary[1]);
    FREEMANY(dataset->windows.all);
}

RDataset0 dataset0_from_windowing(MANY(RWindowing) windowings, const size_t applies_number) {
    Index counter;
    memset(&counter, 0, sizeof(Index));
    for (size_t w = 0; w < windowings.number; w++) {
        RWindowing windowing = windowings._[w];
        counter.all += windowing->windows.number;
        counter.binary[windowing->source->wclass.bc] += windowing->windows.number;
        counter.multi[windowing->source->wclass.mc] += windowing->windows.number;
    }

    Dataset0Init init = {
        .applies_number = applies_number,
        .wsize = windowings._[0]->wsize,
        .windows_counter = counter
    };

    RDataset0 ds = dataset0_alloc(init);

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

    window0s_shuffle(ds->windows.all);
    window0s_shuffle(ds->windows.binary[0]);
    window0s_shuffle(ds->windows.binary[1]);
    DGAFOR(cl) window0s_shuffle(ds->windows.multi[cl]);

    return ds;
}

void datasets0_free() {
    if (datasets_gatherer.root == NULL) {
        return;
    }

    ListItem* cursor = datasets_gatherer.root;

    while (cursor) {
        RDataset0 dataset = cursor->item;
        dataset0_free(dataset);
        cursor = cursor->next;
    }

    list_free(&datasets_gatherer);
}