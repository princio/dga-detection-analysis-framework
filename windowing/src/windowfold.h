
#ifndef __WINDOWFOLD_H__
#define __WINDOWFOLD_H__

#include "common.h"
#include "gatherer2.h"
#include "windowmany.h"

typedef struct WindowFoldK {
    RWindowMC train;
    RWindowMC test;
} WindowFoldK;

MAKEMANY(WindowFoldK);

typedef struct WindowFoldConfig {
    size_t k;
    size_t k_test;
} WindowFoldConfig;

MAKEMANY(WindowFoldConfig);

typedef struct __WindowFold {
    G2Index g2index;

    int isok;
    WindowFoldConfig config;
    MANY(WindowFoldK) foldkmany;
} __WindowFold;

void windowfold_minmax(RWindowFold);

void windowfold_init(RWindowFold, WindowFoldConfig);

RWindowFold windowfold_create(RWindowMC windowmc, const WindowFoldConfig config);

#endif