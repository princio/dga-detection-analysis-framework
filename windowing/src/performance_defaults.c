#include "performance_defaults.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

const Performance performance_defaults[5] = {
    { .name = "F1SCORE_1.0", .func=&performancedefaults_f1score_1, .greater_is_better=1 },
    { .name = "F1SCORE_0.5", .func=&performancedefaults_f1score_05, .greater_is_better=1 },
    { .name = "F1SCORE_0.1", .func=&performancedefaults_f1score_01, .greater_is_better=1 },
    { .name = "FPR", .func=&performancedefaults_fpr, .greater_is_better=0 },
    { .name = "TPR", .func=&performancedefaults_tpr, .greater_is_better=1 },
};

double _performancedefaults_f1score_beta(Detection* detection, TCPC(Performance) performance, double beta) {
    double f1;
    switch (performance->dgadet)
    {
        case PERFORMANCE_DGAHANDLING_MERGE_1: {
            double fp = detection->windows[0][0];
            double fn = detection->windows[2][0] + detection->windows[1][0];
            double tp = detection->windows[2][1] + detection->windows[1][1];

            double beta_2 = beta * beta;

            f1 = ((double) (1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }

        case PERFORMANCE_DGAHANDLING_IGNORE_1: {
            double fp = detection->windows[0][0];
            double fn = detection->windows[2][0];
            double tp = detection->windows[2][1];

            double beta_2 = beta * beta;

            f1 = ((double) (1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }
    }

    return f1;
}

double performancedefaults_f1score_1(Detection* detection, TCPC(Performance) performance) {
    return _performancedefaults_f1score_beta(detection, performance, 1.0);
}

double performancedefaults_f1score_05(Detection* detection, TCPC(Performance) performance) {
    return _performancedefaults_f1score_beta(detection, performance, 0.5);
}

double performancedefaults_f1score_01(Detection* detection, TCPC(Performance) performance) {
    return _performancedefaults_f1score_beta(detection, performance, 0.1);
}

double performancedefaults_fpr(Detection* detection, TCPC(Performance) performance) {
    UNUSED(performance);

    double fp = detection->windows[0][0];
    double tn = detection->windows[0][1];

    return ((double) fp) / (fp + tn);
}

double performancedefaults_tpr(Detection* detection, TCPC(Performance) performance) {
    double fn, tp;

    switch (performance->dgadet)
    {
        case PERFORMANCE_DGAHANDLING_MERGE_1: {
            fn = detection->windows[2][0] + detection->windows[1][0];
            tp = detection->windows[2][1] + detection->windows[1][1];
            break;
        }
        case PERFORMANCE_DGAHANDLING_IGNORE_1: {
            fn = detection->windows[2][0];
            tp = detection->windows[2][1];
            break;
        }
    }

    return ((double) fn) / (fn + tp);
}