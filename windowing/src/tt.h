
#ifndef __TT_H__
#define __TT_H__

#include "common.h"

#include "windows0.h"

typedef struct TTWindow0 {
    MANY(RWindow0) train;
    MANY(RWindow0) test;
} TTWindow0;

typedef TTWindow0 TT0[N_DGACLASSES];

MAKEMANY(TT0);

#endif