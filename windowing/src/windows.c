
#include "windows.h"

#include "gatherer.h"
// #include "logger.h"

#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

MAKEMANY(__Window);

RGatherer windows_gatherer = NULL;

void windows_free(void* item) {
    MANY(__Window)* windows = (MANY(__Window)*) item;

    for (size_t w = 0; w < windows->number; w++) {
        MANY_FREE(windows->_[w].applies);
    }

    MANY_FREE((*windows));
}

void windows_shuffle(MANY(RWindow) rwindows) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    if (rwindows.number > 1) {
        for (size_t i = rwindows.number - 1; i > 0; i--) {
            int32_t j = (unsigned int) (drand48() * (i + 1));
            RWindow t = rwindows._[j];
            rwindows._[j] = rwindows._[i];
            rwindows._[i] = t;
        }
    }
}

MANY(RWindow) windows_alloc(size_t windows_number) {
    MANY(__Window)* windows;
    MANY(RWindow) rwindows;

    if (windows_gatherer == NULL) {
        LOG_DEBUG("Gatherer: creating windows.");
        gatherer_alloc(&windows_gatherer, "windows", windows_free, 100, sizeof(MANY(__Window)), 10);
    }

    windows = gatherer_alloc_item(windows_gatherer);

    MANY_INITREF(windows,  windows_number, __Window);
    MANY_INIT(rwindows, windows_number, RWindow);

    for (size_t w = 0; w < rwindows.number; w++) {
        windows->_[w].index = w;
        windows->_[w].fn_req_min = 0;
        windows->_[w].fn_req_max = 0;
        windows->_[w].windowing = NULL;
        rwindows._[w] = &windows->_[w];
    }

    return rwindows;
}
