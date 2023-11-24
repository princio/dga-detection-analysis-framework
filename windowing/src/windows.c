
#include "windows.h"

#include "list.h"

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

MAKEMANY(__Window);

List* gatherer;

MANY(RWindow) windows_alloc(const int32_t num) {
    MANY(__Window) windows;
    INITMANY(windows, num, __Window);

    MANY(RWindow) rwindows;
    INITMANY(rwindows, num, RWindow);

    if (gatherer == NULL) {
        list_init(gatherer, sizeof(MANY(__Window)));
    }

    list_insert(gatherer, &windows);

    for (int32_t w = 0; w < rwindows.number; w++) {
        rwindows._[w] = &windows._[w];
    }
    
    return rwindows;
}

void windows_free() {
    if (gatherer == NULL) {
        return;
    }

    ListItem* cursor = gatherer->root;

    while (cursor) {
        MANY(__Window)* windows = cursor->item;
        free(windows->_);
        cursor = cursor->next;
    }

    list_free(gatherer, 0);
}

void windows_calc(const DNSMessage message, TCPC(PSet) pset, RWindow window) {
    int whitelistened = 0;
    double value, logit;

    if (pset->windowing == WINDOWING_Q && message.is_response) {
        return;
    } else
    if (pset->windowing == WINDOWING_R && !message.is_response) {
        return;
    }

    logit = message.logit;
    value = message.value;

    if (message.top10m > 0 && message.top10m < pset->whitelisting.rank) {
        value = 0;
        logit = pset->whitelisting.value;
        whitelistened = 1;
    }
    if (logit == INFINITY) {
        logit = pset->infinite_values.pinf;
    } else
        if (logit == (-1 * INFINITY)) {
        logit = pset->infinite_values.ninf;
    }

    ++window->wcount;
    window->dn_bad_05 += value >= 0.5;
    window->dn_bad_09 += value >= 0.9;
    window->dn_bad_099 += value >= 0.99;
    window->dn_bad_0999 += value >= 0.999;

    window->logit += logit;
    window->whitelistened += whitelistened;
}
