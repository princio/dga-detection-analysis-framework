
#include "wapply.h"

#include "common.h"
#include "io.h"
#include "windows.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void wapply_init(RWindow window0, const size_t psets_number) {
    const size_t old_applies = window0->applies.number;
    
    window0->applies.number = psets_number;
    window0->applies._ = realloc(window0->applies._, psets_number * sizeof(WApply));

    memset(&window0->applies._[old_applies], 0, (psets_number - old_applies) * sizeof(WApply));
}

void wapply_init_many(MANY(RWindow) windows, MANY(WSize) wsizes, MANY(PSet) psets) {
    for (size_t i = 0; i < wsizes.number; i++) {
        wapply_init(windows._[i], psets.number);
    }
}

void wapply_run(WApply* wapply, TCPC(DNSMessage) message, TCPC(PSet) pset) {
    int whitelistened = 0;
    double value, logit;

    if (pset->windowing == WINDOWING_Q && message->is_response) {
        return;
    } else
    if (pset->windowing == WINDOWING_R && !message->is_response) {
        return;
    }

    value = message->value;

    if (pset->nx_epsilon_increment >= 0 && message->rcode == 3) {
        value += pset->nx_epsilon_increment;
        value = value >= 1 ? 1 : value;
    }

    if (value == 1 || value == -1) {
        logit = value * INFINITY;
    } else {
        logit = log(value / (1 - value));
    }
    
    if (message->top10m > 0 && ((size_t) message->top10m) < pset->wl_rank) {
        value = 0;
        logit = pset->wl_value;
        whitelistened = 1;
    }
    if (logit == INFINITY) {
        logit = pset->pinf;
    } else
        if (logit == (-1 * INFINITY)) {
        logit = pset->ninf;
    }

    ++wapply->wcount;
    wapply->dn_bad_05 += value >= 0.5;
    wapply->dn_bad_09 += value >= 0.9;
    wapply->dn_bad_099 += value >= 0.99;
    wapply->dn_bad_0999 += value >= 0.999;

    wapply->logit += logit;
    wapply->whitelistened += whitelistened;
}

void wapply_run_many(MANY(WApply)* applies, TCPC(DNSMessage) message, TCPC(MANY(PSet)) psets) {
    for (size_t p = 0; p < psets->number; p++) {
        wapply_run(&applies->_[psets->_[p].index], message, &psets->_[p]);
    }
}
