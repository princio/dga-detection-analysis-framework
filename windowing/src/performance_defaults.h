
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

double performancedefaults_f1score_1(Detection*, TCPC(Performance));
double performancedefaults_f1score_05(Detection*, TCPC(Performance));
double performancedefaults_f1score_01(Detection*, TCPC(Performance));

double performancedefaults_fpr(Detection*, TCPC(Performance));
double performancedefaults_tpr(Detection*, TCPC(Performance));

#endif