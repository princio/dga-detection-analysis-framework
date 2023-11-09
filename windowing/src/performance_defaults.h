
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

double performancedefaults_f1score_1(TCPC(Detection) detection, Performance* performance);
double performancedefaults_f1score_05(TCPC(Detection) detection, Performance* performance);
double performancedefaults_f1score_01(TCPC(Detection) detection, Performance* performance);

double performancedefaults_fpr(TCPC(Detection) detection, Performance* performance);
double performancedefaults_tpr(TCPC(Detection) detection, Performance* performance);

#endif