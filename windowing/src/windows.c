
#include "windows.h"

#include <stdlib.h>
#include <sys/time.h>

void rwindows_from(MANY(Window) windows, MANY(RWindow)* rwindows) {
    rwindows->number = windows.number;
    rwindows->_ = calloc(rwindows->number, sizeof(Window*));
    
    for (int32_t w = 0; w < rwindows->number; w++) {
        rwindows->_[w] = &windows._[w];
    }
}

void rwindows_shuffle(MANY(RWindow)* rwindows) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    if (rwindows->number > 1) {
        for (int32_t i = rwindows->number - 1; i > 0; i--) {
            int32_t j = (unsigned int) (drand48() * (i + 1));
            RWindow t = rwindows->_[j];
            rwindows->_[j] = rwindows->_[i];
            rwindows->_[i] = t;
        }
    }
}

