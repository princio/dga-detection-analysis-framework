
#include "windows.h"

#include <stdlib.h>
#include <sys/time.h>

MANY(RWindow) rwindows_from(MANY(RWindow) rwindows_src) {
    MANY(RWindow) rwindows;
    Window** _windows;
    
    _windows = calloc(rwindows_src.number, sizeof(Window*));
    
    for (int32_t w = 0; w < rwindows_src.number; w++) {
        _windows[w] = rwindows_src._[w];
    }

    rwindows.number = rwindows_src.number;
    rwindows._ = _windows;

    return rwindows;
}

void rwindows_shuffle(MANY(RWindow) rwindows) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    if (rwindows.number > 1) {
        for (int32_t i = rwindows.number - 1; i > 0; i--) {
            int32_t j = (unsigned int) (drand48() * (i + 1));
            RWindow t = rwindows._[j];
            rwindows._[j] = rwindows._[i];
            rwindows._[i] = t;
        }
    }
}

