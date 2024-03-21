#ifndef __WINDOWING_WINDOWDETECTION_H__
#define __WINDOWING_WINDOWDETECTION_H__

#include "common.h"
#include "detect.h"
#include "gatherer2.h"
#include "windowmany.h"

typedef struct __WindowDetection {
    G2Index g2index;

    RWindowSplit windowdetection;

    MANY(Detection) train;
    MANY(Detection) test;
} __WindowDetection;

void windowdetection_run();

#endif