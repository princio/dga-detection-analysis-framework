#ifndef __RESULT_H__
#define __RESULT_H__

#include "common.h"

#include "detect.h"
#include "kfold.h"


typedef struct Test {
    Performance thchooser;
    
    double th;

    struct {
        Detection train[N_DGACLASSES];
        Detection test[N_DGACLASSES];
    } fulldetections;
} Test;

#endif