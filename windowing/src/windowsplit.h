#ifndef __WINDOWING_WINDOWSPLIT_H__
#define __WINDOWING_WINDOWSPLIT_H__

#include "common.h"
#include "gatherer2.h"
#include "windowmany.h"

enum WindowSplitHow {
    WINDOWSPLIT_HOW_BY_DAY,
    WINDOWSPLIT_HOW_PORTION
};

typedef struct WindowSplitConfig {
    enum WindowSplitHow how;
    float portion;
    int day;
    int try;
    int k;
    int k_total;
} WindowSplitConfig;

MAKEMANY(WindowSplitConfig);

typedef struct __WindowSplit {
    G2Index g2index;

    WindowSplitConfig config;

    WSize wsize;

    RWindowMany train; // class 0
    RWindowMC test; // all classes
} __WindowSplit;

void windowsplit_shuffle(RWindowSplit);

RWindowSplit windowsplit_createby_day(const int);

RWindowSplit windowsplit_createby_portion(RWindowMC, const size_t, const size_t);

void windowsplit_detect(RWindowSplit, size_t const, Detection*);

#endif