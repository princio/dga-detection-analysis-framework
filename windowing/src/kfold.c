#include "kfold.h"

#include "cache.h"
#include "io.h"
#include "testbed.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define FALSERATIO(tf) (((double) (tf).falses) / ((tf).falses + (tf).trues))
#define TRUERATIO(tf) (((double) (tf).trues) / ((tf).falses + (tf).trues))


#define MIN(A, B) (A.min) = ((B) <= (A.min) ? (B) : (A.min))
#define MAX(A, B) (A.max) = ((B) >= (A.max) ? (B) : (A.max))

void kfold_splits(MANY(RWindow) rwindows_src, MANY(TT) tts, DGAClass cl, KFoldConfig config, const int32_t _max_windows) {
    const int32_t KFOLDs = config.kfolds;
    const int32_t TRAIN_KFOLDs = config.kfolds - config.test_folds;
    const int32_t TEST_KFOLDs = config.test_folds;

    const int32_t max_windows = (_max_windows != 0) && (_max_windows < rwindows_src.number) ? _max_windows : rwindows_src.number;

    const int32_t kfold_size = max_windows / KFOLDs;
    const int32_t kfold_size_rest = max_windows - (kfold_size * KFOLDs);

    int32_t kindexes[KFOLDs][KFOLDs]; {
        for (int32_t k = 0; k < KFOLDs; k++) {
            for (int32_t kindex = k; kindex < (k + TRAIN_KFOLDs); kindex++) {
                kindexes[k][kindex % KFOLDs] = 0;
            }
            for (int32_t kindex = k + TRAIN_KFOLDs; kindex < (k + KFOLDs); kindex++) {
                kindexes[k][kindex % KFOLDs] = 1;
            }
        }
    }

    for (int32_t k = 0; k < KFOLDs; k++) {
        
        int32_t train_index;
        int32_t test_index;

        const int32_t train_size = kfold_size * TRAIN_KFOLDs + (!kindexes[k][KFOLDs - 1] ? kfold_size_rest : 0);
        const int32_t test_size = kfold_size * TEST_KFOLDs + (kindexes[k][KFOLDs - 1] ? kfold_size_rest : 0);

        T_PC(MANY(RWindow)) drw_train = &tts._[k][cl].train;
        T_PC(MANY(RWindow)) drw_test = &tts._[k][cl].test;

        INITMANYREF(drw_train, train_size, RWindow);
        INITMANYREF(drw_test, test_size, RWindow);

        train_index = 0;
        test_index = 0;
        for (int32_t kk = 0; kk < KFOLDs; kk++) {

            int32_t start = kk * kfold_size;
            int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

            if (kindexes[k][kk] == 0) {
                memcpy(&drw_train->_[train_index], &rwindows_src._[start], sizeof(Window*) * nwindows);
                train_index += nwindows;
            } else {
                memcpy(&drw_test->_[test_index], &rwindows_src._[start], sizeof(Window*) * nwindows);
                test_index += nwindows;
            }
        }
    }
}

KFold kfold_run(const Dataset ds, KFoldConfig config, PSet* pset) {
    assert((config.balance_method != FOLDING_DSBM_EACH) || (config.split_method != FOLDING_DSM_IGNORE_1));

    KFold kfold;
    MANY(TT) tts;

    if (config.kfolds <= 1 || config.kfolds - 1 < config.test_folds) {
        fprintf(stderr, "Error within kfolds (%d) and test_folds (%d).", config.kfolds, config.test_folds);
        exit(1);
    }

    INITMANY(tts, config.kfolds, TT);

    kfold.config = config;
    kfold.pset = pset;
    kfold.ks = tts;

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        MANY(RWindow) rwindows;

        INITMANY(rwindows, ds[cl].number, RWindow);

        memcpy(rwindows._, ds[cl]._, sizeof(RWindow) * ds[cl].number);

        if (config.shuffle) {
            rwindows_shuffle(rwindows);
        }

        kfold_splits(rwindows, tts, cl, config, 0);

        free(rwindows._);
    }

    return kfold;
}

void kfold_free(KFold* kfold) {
    for (int32_t k = 0; k < kfold->config.kfolds; k++) {
        DGAFOR(cl) {
            free(kfold->ks._[k][cl].train._);
            free(kfold->ks._[k][cl].test._);
        }
    }

    free(kfold->ks._);
}

void kfold_io(IOReadWrite rw, FILE* file, void* obj) {
    KFold* kfold = obj;

    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(kfold->config.balance_method);
    FRW(kfold->config.kfolds);
    FRW(kfold->config.shuffle);
    FRW(kfold->config.split_method);
    FRW(kfold->config.test_folds);

    if (rw == IO_READ) {
        INITMANY(kfold->ks, kfold->config.kfolds, TT);
    }

    for (int k = 0; k < kfold->config.kfolds; ++k) {
        for (int l = 0; l < 2; ++l) {
            DGAFOR(cl) {
                MANY(RWindow)* mrw;

                if (l) {
                    mrw = &kfold->ks._[k][cl].train;
                } else {
                    mrw = &kfold->ks._[k][cl].test;
                }

                FRW(mrw->number);

                for (int i = 0; i < mrw->number; ++i) {
                    RWindow window = mrw->_[i];

                    FRW(window->source_index);
                    FRW(window->pset_index);
                    FRW(window->wnum);
                    FRW(window->dgaclass);
                    FRW(window->wcount);
                    FRW(window->logit);
                    FRW(window->whitelistened);
                    FRW(window->dn_bad_05);
                    FRW(window->dn_bad_09);
                    FRW(window->dn_bad_099);
                    FRW(window->dn_bad_0999);
                }
            }
        }
    }
}

void kfold_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    char tb_objid[IO_OBJECTID_LENGTH];
    char tb_objid_subdigest[IO_SUBDIGEST_LENGTH];

    TCPC(KFold) kfold = obj;

    memset(objid, 0, IO_OBJECTID_LENGTH);

    testbed_io_objid(kfold->config.testbed, tb_objid);

    io_subdigest(tb_objid, strlen(tb_objid), tb_objid_subdigest);

    snprintf(objid, IO_OBJECTID_LENGTH, "kfold_%s_%d_%d_", tb_objid_subdigest, kfold->config.kfolds, kfold->pset->id);

    io_appendtime(objid, IO_OBJECTID_LENGTH);
}

void kfold_io_txt(TCPC(void) obj) {
    char testbed_objid[IO_OBJECTID_LENGTH];
    char objid[IO_OBJECTID_LENGTH];
    TCPC(KFold) kfold = obj;
    char fname[800];
    FILE* file;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    kfold_io_objid(kfold, objid);

    testbed_io_objid(kfold->config.testbed, testbed_objid);
    
    sprintf(fname, "%s/%s.md", ROOT_PATH, objid);

    file = io_openfile(IO_WRITE, fname);

    fprintf(file, "# KFold %s\n\n", objid);

    fprintf(file, "Run at: %04d/%02d/%02d %02d:%02d:%02d\n\n", (tm.tm_year + 1900), tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(file, "KFolds: %d\n", kfold->config.kfolds);
    fprintf(file, "KFolds reserved for test split: %d\n", kfold->config.test_folds);
    fprintf(file, "Shuffle: %d\n", kfold->config.shuffle);

    fclose(file);
}