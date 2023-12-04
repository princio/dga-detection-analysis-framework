
#include "windows.h"

#include "list.h"

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

MAKEMANY(__Window);

List windows_gatherer = { .root = NULL };

MANY(double) windows_ths(const MANY(RWindow0) windows[N_DGACLASSES], const size_t pset_index) {
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

MANY(RWindow) windows_alloc(const int32_t num) {
    MANY(__Window) windows;
    INITMANY(windows, num, __Window);

    MANY(RWindow) rwindows;
    INITMANY(rwindows, num, RWindow);

    if (windows_gatherer.root == NULL) {
        list_init(&windows_gatherer, sizeof(MANY(__Window)));
    }

    list_insert_copy(&windows_gatherer, &windows);

    for (size_t w = 0; w < rwindows.number; w++) {
        rwindows._[w] = &windows._[w];
    }
    
    return rwindows;
}

void windows_free() {
    if (windows_gatherer.root == NULL) {
        return;
    }

    ListItem* cursor = windows_gatherer.root;

    while (cursor) {
        MANY(__Window)* windows = cursor->item;
        free(windows->_);
        cursor = cursor->next;
    }

    list_free(&windows_gatherer);
}