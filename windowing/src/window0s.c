
#include "window0s.h"

#include "list.h"

#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

MAKEMANY(__Window0);

List window0s_gatherer = {.root = NULL};

MANY(double) window0s_ths(const MANY(RWindow0) windows[N_DGACLASSES], const size_t pset_index) {
    MANY(double) ths;
    int max;
    double* ths_tmp;

    max = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        max += windows[cl].number;
    }

    ths_tmp = calloc(max, sizeof(double));

    int n = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        for (size_t w = 0; w < windows[cl].number; w++) {
            int logit;
            int exists;

            logit = floor(windows[cl]._[w]->applies._[pset_index].logit);
            exists = 0;
            for (int32_t i = 0; i < n; i++) {
                if (ths_tmp[i] == logit) {
                    exists = 1;
                    break;
                }
            }

            if(!exists) {
                ths_tmp[n++] = logit;
            }
        }
    }

    ths.number = n;
    ths._ = calloc(n, sizeof(double));
    memcpy(ths._, ths_tmp, sizeof(double) * n);

    free(ths_tmp);

    return ths;
}

MANY(RWindow0) rwindow0s_from(MANY(RWindow0) rwindows_src) {
    MANY(RWindow0) rwindows;
    RWindow0* _windows;
    
    _windows = calloc(rwindows_src.number, sizeof(RWindow0));
    
    for (size_t w = 0; w < rwindows_src.number; w++) {
        _windows[w] = rwindows_src._[w];
    }

    rwindows.number = rwindows_src.number;
    rwindows._ = _windows;

    return rwindows;
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

MANY(RWindow0) window0s_alloc(const int32_t num) {
    MANY(__Window0) windows;
    INITMANY(windows, num, __Window0);

    MANY(RWindow0) rwindows;
    INITMANY(rwindows, num, RWindow0);

    if (window0s_gatherer.root == NULL) {
        list_init(&window0s_gatherer, sizeof(MANY(__Window0)));
    }

    list_insert_copy(&window0s_gatherer, &windows);

    for (size_t w = 0; w < rwindows.number; w++) {
        windows._[w].index = w;
        windows._[w].fn_req_min = 0;
        windows._[w].fn_req_max = 0;
        windows._[w].applies_number = 0;
        windows._[w].windowing = NULL;
        rwindows._[w] = &windows._[w];
    }
    
    return rwindows;
}

void window0s_free() {
    if (window0s_gatherer.root == NULL) {
        return;
    }

    ListItem* cursor = window0s_gatherer.root;

    while (cursor) {
        MANY(__Window0)* windows = cursor->item;
        for (size_t w = 0; w < windows->number; w++) {
            FREEMANY(windows->_[w].applies);
        }
        free(windows->_);
        cursor = cursor->next;
    }

    list_free(&window0s_gatherer);
}
