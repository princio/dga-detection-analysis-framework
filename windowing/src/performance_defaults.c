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

double _performancedefaults_f1score_beta(Detection fulldetection[N_DGACLASSES], TCPC(Performance) performance, double beta) {
    double f1;
    switch (performance->dgadet)
    {
        case PERFORMANCE_DGAHANDLING_MERGE_1: {
            double fp = fulldetection[0].windows.falses;
            double fn = fulldetection[2].windows.falses + fulldetection[1].windows.falses;
            double tp = fulldetection[2].windows.trues + fulldetection[1].windows.trues;

            double beta_2 = beta * beta;

            f1 = ((double) (1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }

        case PERFORMANCE_DGAHANDLING_IGNORE_1: {
            double fp = fulldetection[0].windows.falses;
            double fn = fulldetection[2].windows.falses;
            double tp = fulldetection[2].windows.trues;

            double beta_2 = beta * beta;

            f1 = ((double) (1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }
    }

    return f1;
}

double performancedefaults_f1score_1(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    return _performancedefaults_f1score_beta(detection, performance, 1.0);
}

double performancedefaults_f1score_05(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    return _performancedefaults_f1score_beta(detection, performance, 0.5);
}

double performancedefaults_f1score_01(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    return _performancedefaults_f1score_beta(detection, performance, 0.1);
}

double performancedefaults_fpr(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    UNUSED(performance);

    double fp = detection[0].windows.falses;
    double tn = detection[0].windows.trues;

    return ((double) fp) / (fp + tn);
}

double performancedefaults_tpr(Detection detection[N_DGACLASSES], TCPC(Performance) performance) {
    double fn, tp;

    switch (performance->dgadet)
    {
        case PERFORMANCE_DGAHANDLING_MERGE_1: {
            fn = detection[2].windows.falses + detection[1].windows.falses;
            tp = detection[2].windows.trues + detection[1].windows.trues;
            break;
        }
        case PERFORMANCE_DGAHANDLING_IGNORE_1: {
            fn = detection[2].windows.falses;
            tp = detection[2].windows.trues;
            break;
        }
    }

    return ((double) fn) / (fn + tp);
}