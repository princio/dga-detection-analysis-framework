#include "calculator.h"

#include <stdio.h>
#include <math.h>

void calculator_message(Message* message, Window *window, PSet* pset) {
    int whitelistened = 0;
    double value, logit;

    if (pset->windowing == WINDOWING_Q && message->is_response) {
        return;
    } else
    if (pset->windowing == WINDOWING_R && !message->is_response) {
        return;
    }

    // if (wnum != window->wnum) {
    //     printf("Errorrrrrr:\t%d\t%d\n", wnum, window->wnum);
    // }

    logit = message->logit;
    value = message->value;

    if (message->top10m > 0 && message->top10m < pset->whitelisting.rank) {
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

    if (pset->logit_range.min > window->logit) {
        pset->logit_range.min = window->logit;
    }
    if (pset->logit_range.max < window->logit) {
        pset->logit_range.max = window->logit;
    }
}