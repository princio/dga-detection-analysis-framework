#include "calculator.h"

#include <math.h>

void calculator_message(Message* message, const int N_METRICS, WindowMetrics metrics[N_METRICS]) {

    for (int w2 = 0; w2 < N_METRICS; w2++) {
        int whitelistened = 0;
        double value, logit;
        WindowMetrics *metrics;
        PSets pi;
        
        pi = metrics->pi;

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

        ++metrics->wcount;
        metrics->logit += logit;
        metrics->whitelistened += whitelistened;
        metrics->dn_bad_05 += value >= 0.5;
        metrics->dn_bad_09 += value >= 0.9;
        metrics->dn_bad_099 += value >= 0.99;
        metrics->dn_bad_0999 += value >= 0.999;

        if (pi->logit_range.min > metrics->logit) {
            pi->logit_range.min = metrics->logit;
        }
        if (pi->logit_range.max < metrics->logit) {
            pi->logit_range.max = metrics->logit;
        }
    }
}