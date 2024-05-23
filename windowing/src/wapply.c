
#include "wapply.h"

#include "common.h"
#include "io.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const double WApplyDNBad_Values[N_DETBOUND] = { 0, 0.1, 0.25, 0.5, 0.9, 1.1  };

// const size_t WhitelistingRank_Values[N_WHITELISTINGRANK] = { 0, 1000, 10000, 100000, 1000000 };
const size_t WhitelistingRank_Values[N_WHITELISTINGRANK] = { 0, 10, 15, 50, 100 };

void wapply_grouped_run(WApply* wapply, DNSMessageGrouped* message, Config* config) {
    int whitelistened;
    int32_t multiplier;
    double logit, logit_nx;

    whitelistened = 1;
    multiplier = 1;
    logit_nx = 0;
    
    const double value = message->value[config->nn];

    if (0 == config->unique) {
        if (config->windowing == WINDOWING_Q) {
            multiplier = message->q;
        } else
        if (config->windowing == WINDOWING_R) {
            multiplier = message->r;
        } else
        if (config->windowing == WINDOWING_QR) {
            multiplier = message->count;
        }
    }

    {
        int shouldbe_gt0 = 0;
        for (size_t idxdnbad = 0; idxdnbad < N_DETZONE; idxdnbad++) {
            if ((value >= WApplyDNBad_Values[idxdnbad]) && (value < WApplyDNBad_Values[idxdnbad + 1])) {
                wapply->dn_bad[idxdnbad] += multiplier;
                shouldbe_gt0++;
            }
        }
        assert(shouldbe_gt0);
    }

    if (value == 1 || value == -1) {
        logit = value * INFINITY;
        if (value == 1) {
            logit = config->pinf;
        } else {
            logit = config->ninf;
        }
    } else {
        logit = log(value / (1 - value));
    }

    if (message->top10m > 0 && ((size_t) message->top10m) < config->wl_rank) {
        logit = config->wl_value;
        whitelistened = 1;
    }

    if (config->nx_logit_increment > 0) {
        logit_nx = logit + config->nx_logit_increment;
    }

    ++wapply->wcount;
    wapply->logit += logit * multiplier + logit_nx;
    wapply->whitelistened_unique += whitelistened;
    wapply->whitelistened_total += multiplier;
}

void wapply_run(WApply* wapply, DNSMessage* message, Config* config) {
    int whitelistened = 0;
    double value, logit;

    if (config->windowing == WINDOWING_Q && message->is_response) {
        return;
    } else
    if (config->windowing == WINDOWING_R && !message->is_response) {
        return;
    }

    value = message->value[config->nn];

    if (config->nx_logit_increment > 0 && message->rcode == 3) {
        value += config->nx_logit_increment;
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

    int a = 0;
    for (size_t idxdnbad = 0; idxdnbad < N_DETZONE; idxdnbad++) {
        if ((value >= WApplyDNBad_Values[idxdnbad]) && (value < WApplyDNBad_Values[idxdnbad + 1])) {
            wapply->dn_bad[idxdnbad]++;
            a++;
        }
    }
    assert(a);

    if (fabs(logit - message->logit[config->nn]) > 0.001) {
        printf("%f - %f = %f\n", logit, message->logit[config->nn], logit - message->logit[config->nn]);
    }
    wapply->logit += logit;
    wapply->whitelistened_total += whitelistened;
}