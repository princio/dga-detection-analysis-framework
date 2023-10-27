#include "folding.h"

#include "dataset.h"

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

void folding_generate(Dataset* ds, Folding* folding) {
    assert((folding->config.balance_method != FOLDING_DSBM_EACH) || (folding->config.split_method != FOLDING_DSM_IGNORE_1));

    RWindows rwindows[N_DGACLASSES];
    folding->ks = calloc(folding->config.kfolds, sizeof(FoldingK));

    dataset_rwindows(ds, rwindows, folding->config.shuffle);

    const int32_t KFOLDs = folding->config.kfolds;
    const int32_t TRAIN_KFOLDs = folding->config.kfolds - folding->config.test_folds;
    const int32_t TEST_KFOLDs = folding->config.test_folds;

    int32_t kindexes[KFOLDs][KFOLDs];
    for (int32_t k = 0; k < KFOLDs; k++) {
        for (int32_t kindex = k; kindex < (k + TRAIN_KFOLDs); kindex++) {
            kindexes[k][kindex % KFOLDs] = 0;
        }
        for (int32_t kindex = k + TRAIN_KFOLDs; kindex < (k + KFOLDs); kindex++) {
            kindexes[k][kindex % KFOLDs] = 1;
        }
    }


    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        int32_t kfold_size = rwindows[cl].number / KFOLDs;
        int32_t kfold_size_rest = rwindows[cl].number - (kfold_size * KFOLDs);

        for (int32_t k = 0; k < KFOLDs; k++) {

            int32_t train_size;
            int32_t test_size;
            FoldingK* fk = &folding->ks[k];

            fk->n_sources[cl] = ds->sources.arrays[cl].number;

            detect_evaluations_init(fk->train.evaluations, fk->n_sources);
            detect_evaluations_init(fk->test.evaluations, fk->n_sources);

            train_size = kfold_size * TRAIN_KFOLDs;
            test_size = kfold_size * TEST_KFOLDs;
            fk = &folding->ks[k];

            RWindows* drw_train = fk->train.rwindows;
            RWindows* drw_test = fk->test.rwindows;

            fk->k = k;

            if (kindexes[k][KFOLDs - 1]) {
                test_size += kfold_size_rest;
            } else {
                train_size += kfold_size_rest;
            }

            drw_train[cl].number = train_size;
            drw_train[cl]._ = calloc(train_size, sizeof(Window*));

            drw_test[cl].number = test_size;
            drw_test[cl]._ = calloc(test_size, sizeof(Window*));

            drw_train[cl].number = train_size;
            drw_train[cl]._ = calloc(train_size, sizeof(Window*));

            drw_test[cl].number = test_size;
            drw_test[cl]._ = calloc(test_size, sizeof(Window*));

            int32_t train_index = 0;
            int32_t test_index = 0;
            for (int32_t kk = 0; kk < KFOLDs; kk++) {

                int32_t start = kk * kfold_size;
                int32_t nwindows = kfold_size + (kk == (KFOLDs - 1) ? kfold_size_rest : 0);

                if (kindexes[k][kk] == 0) {
                    memcpy(&drw_train[cl]._[train_index], &rwindows[cl]._[start], sizeof(Window*) * nwindows);
                    train_index += nwindows;
                } else {
                    memcpy(&drw_test[cl]._[test_index], &rwindows[cl]._[start], sizeof(Window*) * nwindows);
                    test_index += nwindows;
                }
            }
        }
    }

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        free(rwindows[cl]._);
    }
}

void folding_k_train(FoldingK* fk) {
    Ths ths;
    dataset_rwindows_ths(fk->train.rwindows, &ths);

    for (int t = 0; t < ths.number; t++) {
        Evaluations evaluations;
        detect_evaluations_init(evaluations, fk->n_sources);
        detect_evaluate_many(fk->train.rwindows, ths._[t], evaluations); // perform the detection with this `th`

        for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
            for (int ev = 0; ev <= N_EVALUATEMETHODs; ev++) {
                Evaluation* eval_best = &fk->train.evaluations[dd][ev];
                Evaluation* eval_th = &evaluations[dd][ev];

                int better = evaluation_methods[ev].greater ? eval_best->score < eval_th->score : eval_best->score > eval_th->score;
                if (better) {
                    memcpy(eval_best, eval_th, sizeof(Evaluation));
                }
            }
        }
    }
    free(ths._);
}

void folding_k_test(FoldingK* fk) {
    for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
        for (int ev = 0; ev <= N_EVALUATEMETHODs; ev++) {
            detect_evaluate(dd, ev, fk->test.rwindows, fk->train.evaluations[dd][ev].th, &fk->test.evaluations[dd][ev]);
        }
    }
}

void folding_run(Dataset* ds, Folding* folding) {
    folding_generate(ds, folding);
    for (int32_t k = 0; k < folding->config.kfolds; k++) {
        folding_k_train(&folding->ks[k]);
        folding_k_test(&folding->ks[k]);
    }
}

void folding_free(Folding* folding) {
    for (int32_t k = 0; k < folding->config.kfolds; k++) {
        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
            free(folding->ks[k].train.rwindows[cl]._);
            free(folding->ks[k].test.rwindows[cl]._);
            detect_evaluations_free(folding->ks[k].train.evaluations);
            detect_evaluations_free(folding->ks[k].test.evaluations);
        }
    }
    free(folding->ks);
    free(folding);
}
