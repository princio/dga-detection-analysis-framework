
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

void wapply_run(WApply* wapply, TCPC(DNSMessage) message, Config* config) {
    int whitelistened = 0;
    double value, logit;

    if (config->windowing == WINDOWING_Q && message->is_response) {
        return;
    } else
    if (config->windowing == WINDOWING_R && !message->is_response) {
        return;
    }

    value = message->value;

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
    wapply->dn_bad_05 += value >= 0.5;
    wapply->dn_bad_09 += value >= 0.9;
    wapply->dn_bad_099 += value >= 0.99;
    wapply->dn_bad_0999 += value >= 0.999;

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