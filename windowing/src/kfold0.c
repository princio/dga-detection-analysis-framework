#include "kfold0.h"

#include "cache.h"
#include "dataset.h"
#include "io.h"
#include "window0s.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


void kfold0_md(char dirname[200], const RKFold0 kfold0);

#define FALSERATIO(tf) (((double) (tf).falses) / ((tf).falses + (tf).trues))
#define TRUERATIO(tf) (((double) (tf).trues) / ((tf).falses + (tf).trues))

#define MIN(A, B) (A.min) = ((B) <= (A.min) ? (B) : (A.min))
#define MAX(A, B) (A.max) = ((B) >= (A.max) ? (B) : (A.max))

RKFold0 kfold0_create(RTestBed2 tb2, KFoldConfig0 config) {
    assert((config.balance_method != FOLDING_DSBM_EACH) || (config.split_method != FOLDING_DSM_IGNORE_1));

    if (config.kfolds <= 1 || config.kfolds - 1 < config.test_folds) {
        fprintf(stderr, "Error within kfolds (%ld) and test_folds (%ld).", config.kfolds, config.test_folds);
        exit(1);
    }

    RKFold0 kfold0;

    kfold0 = calloc(1, sizeof(__KFold0));

    kfold0->testbed2 = tb2;
    kfold0->config = config;
    INITMANY(kfold0->datasets.wsize, tb2->wsizes.number, KFold0WSizeSplits);

    return kfold0;
}

RKFold0 kfold0_run(RTestBed2 tb2, KFoldConfig0 config) {
    RKFold0 kfold0 = kfold0_create(tb2, config);

    for (size_t i = 0; i < tb2->wsizes.number; i++) {
        RDataset0 ds = tb2->datasets.wsize._[i];

        kfold0->datasets.wsize._[i].splits = dataset0_splits(ds, config.kfolds, config.test_folds);
    }

    return kfold0;
}

void kfold0_free(RKFold0 kfold0) {
    // Datasets are free'd within datasets0_free
    for (size_t i = 0; i < kfold0->datasets.wsize.number; i++) {
        FREEMANY(kfold0->datasets.wsize._[i].splits);
    }
    FREEMANY(kfold0->datasets.wsize);
    
    free(kfold0);
}


void kfold0_io(IOReadWrite rw, char dirname[200], RTestBed2 tb2, RKFold0* kfold0) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    if (tb2 == NULL) {
        printf("Warning: loading TestBed2.\n");
        return;
    }

    char fpath[210];
    FILE* file;
    KFoldConfig0 kconfig;

    sprintf(fpath, "%s/kfold0.bin", dirname);

    file = io_openfile(rw, fpath);

    if (file == NULL) {
        printf("Error: file <%s> not opened.\n", fpath);
        return;
    }

    if (rw == IO_WRITE) {
        kconfig = (*kfold0)->config;
    }

    sprintf(fpath, "%s/results.bin", dirname);

    file = io_openfile(rw, fpath);

    FRW(kconfig.balance_method);
    FRW(kconfig.kfolds);
    FRW(kconfig.shuffle);
    FRW(kconfig.split_method);
    FRW(kconfig.test_folds);

    if (rw == IO_READ) {
        (*kfold0) = kfold0_create(tb2, kconfig);
    }

    for (size_t i = 0; i < (*kfold0)->datasets.wsize.number; i++) {
        int is_ok;
        MANY(DatasetSplit0)* ds_splits;

        ds_splits = &(*kfold0)->datasets.wsize._[i].splits;

        if (rw == IO_READ) {
            INITMANYREF(ds_splits, kconfig.kfolds, DatasetSplit0);
        }

        if (rw == IO_WRITE) {
            is_ok = dataset0_splits_ok(*ds_splits);
        }

        FRW(is_ok);

        if (!is_ok) {
            printf("Warning: skipping dataset splits for wsize=%ld.\n",  (*kfold0)->testbed2->wsizes._[i].value);
            continue;
        }

        for (size_t k = 0; k < kconfig.kfolds; k++) {
            testbed2_io_dataset(rw, file, tb2, i, &ds_splits->_[k].train);
            testbed2_io_dataset(rw, file, tb2, i, &ds_splits->_[k].test);
        }
    }

    fclose(file);

    if (rw == IO_WRITE) {
        kfold0_md(dirname, *kfold0);
    }
}


void kfold0_md(char dirname[200], const RKFold0 kfold0) {
    char fpath[210];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    FILE* file;
    sprintf(fpath, "%s/kfold0.md", dirname);
    file = fopen(fpath, "w");

    t = time(NULL);
    tm = *localtime(&t);

    #define FP(...) fprintf(file, __VA_ARGS__);
    #define FPNL(N, ...) fprintf(file, __VA_ARGS__); for (size_t i = 0; i < N; i++) fprintf(file, "\n");

    FPNL(2, "# KFold0");

    FPNL(3, "Saved on date: %d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    FPNL(2, "## Configuration");

    FPNL(1, "- K=%ld", kfold0->config.kfolds);
    FPNL(1, "- K for testing: %ld", kfold0->config.test_folds);

    {
        const int len_header = 15;
        const char* headers[] = {
            "wsize",
            "folded",
            "0 train avg",
            "0 test avg",
            "1 train avg",
            "1 test avg",
            "2 train avg",
            "2 test avg",
        };
        const size_t n_headers = sizeof (headers) / sizeof (const char *);
        for (size_t i = 0; i < n_headers; i++) {
            FP("|%*s", len_header, headers[i]);
        }
        FPNL(1, "|");
        for (size_t i = 0; i < n_headers; i++) {
            FP("|");
            for (int t = 0; t < len_header; t++) {
                FP("-");
            }
        }
        FPNL(1, "|");
        for (size_t i = 0; i < kfold0->datasets.wsize.number; i++) {
            MANY(DatasetSplit0) splits = kfold0->datasets.wsize._[i].splits;
            FP("|%*ld", len_header, kfold0->testbed2->wsizes._[i].value);
            FP("|%*d", len_header, dataset0_splits_ok(splits));
            DGAFOR(cl) {
                float train_size = 0;
                float test_size = 0;
                float train_logit_avg = 0;
                float test_logit_avg = 0;
                for (size_t k = 0; k < splits.number; k++) {
                    train_size += splits._[k].train->windows.multi[cl].number;
                    test_size += splits._[k].test->windows.multi[cl].number;
                }
                FP("|%*.2f", len_header, train_size / splits.number);
                FP("|%*.2f", len_header, test_size / splits.number);
            }
            FPNL(1, "|");
        }
    }

    #undef FP
    #undef FPNL
}