#include "calculator.h"

#include <stdio.h>
#include <math.h>

void calculator_message(Message* message, WindowMetricSets *metrics, PSet* psets) {

    for (int m = 0; m < metrics->number; m++) {
        int whitelistened = 0;
        double value, logit;
        WindowMetricSet *metric = &metrics->_[m];

        PSet* pi = &psets[m];


        if (pi->windowing == WINDOWING_Q && message->is_response) {
            continue;
        } else
        if (pi->windowing == WINDOWING_R && !message->is_response) {
            continue;
        }

        // if (wnum != window->wnum) {
        //     printf("Errorrrrrr:\t%d\t%d\n", wnum, window->wnum);
        // }

        logit = message->logit;
        value = message->value;

        if (message->top10m > 0 && message->top10m < pi->whitelisting.rank) {
            value = 0;
            logit = pi->whitelisting.value;
            whitelistened = 1;
        }
        if (logit == INFINITY) {
            logit = pi->infinite_values.pinf;
        } else
            if (logit == (-1 * INFINITY)) {
            logit = pi->infinite_values.ninf;
        }

        ++metric->wcount;
        metric->dn_bad_05 += value >= 0.5;
        metric->dn_bad_09 += value >= 0.9;
        metric->dn_bad_099 += value >= 0.99;
        metric->dn_bad_0999 += value >= 0.999;

        metric->logit = logit;
        metric->whitelistened += whitelistened;

        if (pi->logit_range.min > metric->logit) {
            pi->logit_range.min = metric->logit;
        }
        if (pi->logit_range.max < metric->logit) {
            pi->logit_range.max = metric->logit;
        }
    }
}