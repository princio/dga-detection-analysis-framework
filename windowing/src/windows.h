
#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include "common.h"

typedef struct Window {
    Index index;
    
    IDX pset_index;

    IDX wnum;

    DGAClass dgaclass;

    int32_t wcount;
    double  logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} Window;

typedef Window* RWindow;

MAKEMANY(Window);
MAKEMANY(MANY(Window));
MAKEDGAMANY(Window);

MAKEMANY(RWindow);
MAKEDGAMANY(RWindow);

void rwindows_from(MANY(Window), MANY(RWindow)*);

void rwindows_shuffle(MANY(RWindow));

#endif