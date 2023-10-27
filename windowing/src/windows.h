
#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include "common.h"

typedef struct Window {
    IDX source_index;
    IDX source_classindex;
    IDX dataset_id;
    IDX window_id;

    DGAClass dgaclass;

    int32_t wcount;
    double  logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} Window;

typedef struct Windows {
    int32_t number;
    Window* _;
} Windows;

typedef Window* RWindow;

typedef struct RWindows {
    int32_t number;
    RWindow* _;
} RWindows;


void rwindows_from(Windows, RWindows*);

void rwindows_shuffle(RWindows*);


#endif