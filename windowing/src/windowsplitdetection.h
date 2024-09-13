
#ifndef __TRAINER_DETECTIONS_2_H__
#define __TRAINER_DETECTIONS_2_H__

#include "common.h"
#include "configsuite.h"
#include "gatherer2.h"

typedef struct WindowSplitDetection {
    G2Index g2index_split;
    Config config;

    MinMax train;
    MinMax test_all;
    MinMax test_bin[2];
    MinMax test_mul[N_DGACLASSES];
    
    Detection detection;
} WindowSplitDetection;

void* windowsplitdetection_start();
void windowsplitdetection_wait(void*);

#endif