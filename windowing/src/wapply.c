
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

const double WApplyDNBad_Values[N_WAPPLYDNBAD] = { 0.5, 0.9, 0.99, 0.999 };

void wapply_run(WApply* wapply, TCPC(DNSMessage) message, Config* config) {
    int whitelistened = 0;
    double value, logit;

    if (config->windowing == WINDOWING_Q && message->is_response) {
        return;
    } else
    if (config->windowing == WINDOWING_R && !message->is_response) {
        return;
    }

    value = message->value[config->nn];

    if (config->nx_epsilon_increment >= 0 && message->rcode == 3) {
        value += config->nx_epsilon_increment;
        value = value >= 1 ? 1 : value;
    }

    if (value == 1 || value == -1) {
        logit = value * INFINITY;
    } else {
        logit = log(value / (1 - value));
    }
    
    if (message->top10m > 0 && ((size_t) message->top10m) < config->wl_rank) {
        value = 0;
        logit = config->wl_value;
        whitelistened = 1;
    }
    if (logit == INFINITY) {
        logit = config->pinf;
    } else
        if (logit == (-1 * INFINITY)) {
        logit = config->ninf;
    }

    ++wapply->wcount;

    for (size_t idxdnbad = 0; idxdnbad < N_WAPPLYDNBAD; idxdnbad++) {
        wapply->dn_bad[idxdnbad] += (value >= WApplyDNBad_Values[idxdnbad]);
    }

    wapply->logit += logit;
    wapply->whitelistened += whitelistened;
}

void* wapply_run_args(void* argsvoid) {
    WApplyArgs* args = argsvoid;
    printf("Running %d", args->id);
    wapply_run(args->wapply, args->message, args->config);
    printf("Ended %d", args->id);
    return NULL;
}