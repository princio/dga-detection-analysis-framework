
#include "windows0.h"

#include "list.h"

#include <stdlib.h>
#include <sys/time.h>

MAKEMANY(__Window0);

List* gatherer;

MANY(RWindow0) rwindows0_from(MANY(RWindow0) rwindows_src) {
    MANY(RWindow0) rwindows;
    RWindow0* _windows;
    
    _windows = calloc(rwindows_src.number, sizeof(RWindow0));
    
    for (int32_t w = 0; w < rwindows_src.number; w++) {
        _windows[w] = rwindows_src._[w];
    }

    rwindows.number = rwindows_src.number;
    rwindows._ = _windows;

    return rwindows;
}

void windows0_shuffle(MANY(RWindow0) rwindows) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    if (rwindows.number > 1) {
        for (int32_t i = rwindows.number - 1; i > 0; i--) {
            int32_t j = (unsigned int) (drand48() * (i + 1));
            RWindow0 t = rwindows._[j];
            rwindows._[j] = rwindows._[i];
            rwindows._[i] = t;
        }
    }
}

MANY(RWindow0) windows0_alloc(const int32_t num) {
    MANY(__Window0) windows;
    INITMANY(windows, num, __Window0);

    MANY(RWindow0) rwindows;
    INITMANY(rwindows, num, RWindow0);

    if (gatherer == NULL) {
        list_init(gatherer, sizeof(MANY(__Window0)));
    }

    list_insert(gatherer, &windows);

    for (int32_t w = 0; w < rwindows.number; w++) {
        rwindows._[w] = &windows._[w];
    }
    
    return rwindows;
}

void windows0_free() {
    if (gatherer == NULL) {
        return;
    }

    ListItem* cursor = gatherer->root;

    while (cursor) {
        MANY(__Window0)* windows = cursor->item;
        free(windows->_);
        cursor = cursor->next;
    }

    list_free(gatherer, 0);
}

