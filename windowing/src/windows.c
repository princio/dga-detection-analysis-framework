
#include "windows.h"

#include "list.h"

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

MAKEMANY(__Window);

List windows_gatherer = { .root = NULL };

MANY(RWindow) windows_alloc(const int32_t num) {
    MANY(__Window) windows;
    INITMANY(windows, num, __Window);

    MANY(RWindow) rwindows;
    INITMANY(rwindows, num, RWindow);

    if (windows_gatherer.root == NULL) {
        list_init(&windows_gatherer, sizeof(MANY(__Window)));
    }

    list_insert(&windows_gatherer, &windows);

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

    list_free(&windows_gatherer, 0);
}