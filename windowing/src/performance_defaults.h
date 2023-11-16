
#ifndef __PERFORMANCEDEFAULTS_H__
#define __PERFORMANCEDEFAULTS_H__

#include "detect.h"

enum PerformanceFunctionsEnum {
    PERFORMANCEDEFAULTS_F1SCORE_1,
    PERFORMANCEDEFAULTS_F1SCORE_05,
    PERFORMANCEDEFAULTS_F1SCORE_01,
    PERFORMANCEDEFAULTS_FPR,
    PERFORMANCEDEFAULTS_TPR
};

extern const Performance performance_defaults[5];

double performancedefaults_f1score_1(Detection[N_DGACLASSES], TCPC(Performance));
double performancedefaults_f1score_05(Detection[N_DGACLASSES], TCPC(Performance));
double performancedefaults_f1score_01(Detection[N_DGACLASSES], TCPC(Performance));

double performancedefaults_fpr(Detection[N_DGACLASSES], TCPC(Performance));
double performancedefaults_tpr(Detection[N_DGACLASSES], TCPC(Performance));

#endif