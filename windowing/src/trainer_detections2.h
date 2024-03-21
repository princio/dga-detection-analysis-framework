
#ifndef __TRAINER_DETECTIONS_2_H__
#define __TRAINER_DETECTIONS_2_H__

#include "configsuite.h"
#include "gatherer2.h"

typedef struct WindowDetection {
    MANY(Detection) train;
    MANY(Detection) test;
} WindowDetection;

void* windowdetection_run();
void windowdetection_wait(void*);

#endif