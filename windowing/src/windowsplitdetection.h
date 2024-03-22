
#ifndef __TRAINER_DETECTIONS_2_H__
#define __TRAINER_DETECTIONS_2_H__

#include "common.h"
#include "configsuite.h"
#include "gatherer2.h"

typedef struct WindowSplitDetection {
    MANY(Detection) train;
    MANY(Detection) test;
} WindowSplitDetection;

void* windowsplitdetection_run();
void windowsplitdetection_wait(void*);

#endif