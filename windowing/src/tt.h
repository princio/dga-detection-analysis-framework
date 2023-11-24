
#ifndef __TT_H__
#define __TT_H__

#include "common.h"

#include "windows.h"

typedef struct TTWindow {
    MANY(RWindow) train;
    MANY(RWindow) test;
} TTWindow;

typedef struct TTWindow0 {
    MANY(RWindow0) train;
    MANY(RWindow0) test;
} TTWindow0;

typedef TTWindow TT[N_DGACLASSES];

typedef TTWindow0 TT0[N_DGACLASSES];

MAKEMANY(TT);

MAKEMANY(TT0);


typedef struct TetraX {
    size_t all;
    size_t binary[2];
    size_t multi[N_DGACLASSES];
} TetraX;

typedef struct Tetra {
    __MANY all;
    __MANY binary[2];
    __MANY multi[N_DGACLASSES];
} Tetra;

#endif