#include "performance_defaults.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

const Performance performance_defaults[7] = {
    { .name = "F1SCORE_1.0", .func=&performancedefaults_f1score_1, .greater_is_better=1 },
    { .name = "F1SCORE_0.5", .func=&performancedefaults_f1score_05, .greater_is_better=1 },
    { .name = "F1SCORE_0.1", .func=&performancedefaults_f1score_01, .greater_is_better=1 },
    { .name = "FPR", .func=&performancedefaults_fpr, .greater_is_better=0 },
    { .name = "TPR1", .func=&performancedefaults_tpr1, .greater_is_better=1 },
    { .name = "TPR2", .func=&performancedefaults_tpr2, .greater_is_better=1 },
    { .name = "TPR12", .func=&performancedefaults_tpr12, .greater_is_better=1 },
};

double _performancedefaults_f1score_beta(Detection* detection, TCPC(Performance) performance, double beta) {
    double f1 = -1;
    switch (performance->dgadet)
    {
        case PERFORMANCE_DGAHANDLING_MERGE_1: {
            const double fp = PN_FP(detection->windows);
            const double fn = PN_FN(detection->windows);
            const double tp = PN_TP(detection->windows);

            const double beta_2 = beta * beta;

            f1 = ((1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }

        case PERFORMANCE_DGAHANDLING_IGNORE_1: {
            const double fp = PN_FP(detection->windows);
            const double fn = PN_FN_CL(detection->windows, 2);
            const double tp = PN_TP_CL(detection->windows, 2);;

            double beta_2 = beta * beta;

            f1 = ((1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
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
    return PN_FALSERATIO(detection->windows, 0);
}

double performancedefaults_tpr1(Detection* detection, TCPC(Performance) performance) {
    return PN_TRUERATIO(detection->windows, 1);
}

double performancedefaults_tpr2(Detection* detection, TCPC(Performance) performance) {
    return PN_TRUERATIO(detection->windows, 2);
}

double performancedefaults_tpr12(Detection* detection, TCPC(Performance) performance) {
    const double fn = PN_FN(detection->windows);
    const double tp = PN_TP(detection->windows);

    return ((double) tp) / (fn + tp);
}