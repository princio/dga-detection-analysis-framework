#include "tt.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


void _rwindows_ths(const DGAMANY(RWindow) dsrwindows, MANY(DGA)* ths) {
    int max = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        max += dsrwindows[cl].number;
    }

    double* ths_tmp = calloc(max, sizeof(double));

    int n = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        for (int32_t w = 0; w < dsrwindows[cl].number; w++) {
            int logit;
            int exists;

            logit = floor(dsrwindows[cl]._[w]->logit);
            exists = 0;
            for (int32_t i = 0; i < n; i++) {
                if (ths_tmp[i] == logit) {
                    exists = 1;
                    break;
                }
            }

            if(!exists) {
                ths_tmp[n++] = logit;
            }
        }
    }

    ths->number = n;
    ths->_ = calloc(n, sizeof(double));
    memcpy(ths->_, ths_tmp, sizeof(double) * n);

    free(ths_tmp);
}

void tt_evaluations_init(MANY(Performance) performances, const Index nsources, MANY(TTEvaluation)* ttes) {
    INITMANYREF(ttes, performances.number, TTEvaluation);

    for (int32_t p = 0; p < ttes->number; p++) {
        detect_init(&ttes->_[p].train.detection, nsources);
        detect_init(&ttes->_[p].test.detection, nsources);
        ttes->_[p].performance = &performances._[p];
    }
}

void tt_evaluations_free(MANY(TTEvaluation) ttes) {
    for (int32_t p = 0; p < ttes.number; p++) {
        detect_free(&ttes._[p].train.detection);
        detect_free(&ttes._[p].test.detection);
    }
    free(ttes._);
}

void _tt_train(TCPC(TT) tt, MANY(TTEvaluation)* ttes) {
    MANY(DGA) ths;
    Detection _detection;

    {
        _rwindows_ths(tt->train, &ths);
        detect_init(&_detection, tt->nsources);
    }

    printf("ths.number\t%d\n", ths.number);

    int8_t outputs_init[ttes->number];

    memset(outputs_init, 0, ttes->number * sizeof(int8_t));

    for (int t = 0; t < ths.number; t++) {
        detect_run(tt->train, ths._[t], &_detection);

        for (int32_t ev = 0; ev < ttes->number; ev++) {
            double performance_score = detect_performance(&_detection, ttes->_[ev].performance);

            const int is_better = detect_performance_compare(ttes->_[ev].performance, performance_score, ttes->_[ev].train.score);
            if (outputs_init[ev] == 0 || is_better) {
                outputs_init[ev] = 1;
                ttes->_[ev].train.score = performance_score;
            }
        }
    }

    free(ths._);
    detect_free(&_detection);
}

void _tt_test(TCPC(TT) tt, MANY(TTEvaluation)* ttes) {
    for (int32_t i = 0; i < ttes->number; i++) {
        double th;

        th = ttes->_[i].train.detection.th;

        detect_init(&ttes->_[i].test.detection, tt->nsources);
        
        detect_run(tt->test, th, &ttes->_[i].test.detection);
        
        ttes->_[i].test.score = detect_performance(&ttes->_[i].test.detection, ttes->_[i].performance);
    }
}

MANY(TTEvaluation) tt_run(TCPC(TT) tt, MANY(Performance) performances) {
    MANY(TTEvaluation) ttes;
    
    tt_evaluations_init(performances, tt->nsources, &ttes);
    
    _tt_train(tt, &ttes);

    _tt_test(tt, &ttes);

    return ttes;
}

void tt_free(TT tt) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        free(tt.train[cl]._);
        free(tt.test[cl]._);
    }
}