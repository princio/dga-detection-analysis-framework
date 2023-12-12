
#include "window0s.h"

#include "gatherer.h"

#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

MAKEMANY(__Window0);

RGatherer window0s_gatherer = NULL;

void window0s_free(void* item) {
    MANY(__Window0)* windows = (MANY(__Window0)*) item;

    for (size_t w = 0; w < windows->number; w++) {
        FREEMANY(windows->_[w].applies);
    }

    FREEMANY((*windows));
}

void window0s_shuffle(MANY(RWindow0) rwindows) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    if (rwindows.number > 1) {
        for (size_t i = rwindows.number - 1; i > 0; i--) {
            int32_t j = (unsigned int) (drand48() * (i + 1));
            RWindow0 t = rwindows._[j];
            rwindows._[j] = rwindows._[i];
            rwindows._[i] = t;
        }
    }
}

MANY(RWindow0) window0s_alloc(size_t window0s_number) {
    MANY(__Window0)* windows;
    MANY(RWindow0) rwindows;

    if (window0s_gatherer == NULL) {
        gatherer_alloc(&window0s_gatherer, "window0s", window0s_free, 100, sizeof(MANY(__Window0)), 10);
    }

    windows = gatherer_alloc_item(window0s_gatherer);

    INITMANYREF(windows,  window0s_number, __Window0);
    INITMANY(rwindows, window0s_number, RWindow0);

    for (size_t w = 0; w < rwindows.number; w++) {
        windows->_[w].index = w;
        windows->_[w].fn_req_min = 0;
        windows->_[w].fn_req_max = 0;
        windows->_[w].windowing = NULL;
        rwindows._[w] = &windows->_[w];
    }

    return rwindows;
}
