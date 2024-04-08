
#ifndef __WINDOWZONEDETECTION_H__
#define __WINDOWZONEDETECTION_H__

#include "common.h"
#include "configsuite.h"
#include "gatherer2.h"

typedef struct WindowZoneDetection {
    G2Index g2index_zone;
    Config config;

    MinMax train;
    MinMax test_all;
    MinMax test_bin[2];
    MinMax test_mul[N_DGACLASSES];
    
    Detection detection;
} WindowZoneDetection;

void* windowzonedetection_start();
void windowzonedetection_wait(void*);

#endif