
#include "windowing.h"

#include "dn.h"
#include "persister.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void windowing_fetch(WindowingPtr windowing) {
    for (int32_t i = 0; i < windowing->captures.number; ++i) {
        windowing->captures._[i].fetch(windowing, i);
    }
}

void windowing_capture_init(WindowingPtr windowing, int capture_index) {
    Capture* capture = &windowing->captures._[capture_index];
    WSet* capture_wsets = windowing->captures_wsets[capture_index];

    // printf("Windowing initialization of %d capture having %ld dn.\n", capture_index, capture->fnreq_max);

    for (int32_t w = 0; w < windowing->wsizes.number; ++w) {
        int32_t wsize = windowing->wsizes._[w];

        int32_t n_windows = N_WINDOWS(capture->fnreq_max, wsize);

        capture_wsets[w].number = n_windows;
        capture_wsets[w]._ = calloc(n_windows, sizeof(Window));

        Window* windows = capture_wsets[w]._;
        for (int32_t w = 0; w < n_windows; ++w) {

            windows[w].wnum = w;
            windows[w].parent_id = capture_index;
            windows[w].class = capture->class;
            windows[w].metrics.number = windowing->psets.number;
            windows[w].metrics._ = calloc(windowing->psets.number, sizeof(WindowMetricSet));

            for (int32_t m = 0; m < windowing->psets.number; ++m) {
                windows[w].metrics._[m].pi_id = m;
            }
        }
    }
}

void windowing_captures_init(WindowingPtr windowing) {
    for (int capture_index = 0; capture_index < windowing->captures.number; capture_index++) {
        windowing_capture_init(windowing, capture_index);
    }
}


int windowing_compare(WindowingPtr windowing, WindowingPtr windowing_test, int capture_index) {
    int eq = 0;
    for (int ws = 0; ws < windowing->wsizes.number; ws++) {
        WSet* wset = &windowing->captures_wsets[capture_index][ws];
        WSet* wset_test = &windowing_test->captures_wsets[capture_index][ws];

        if (wset->number != wset_test->number) {
            printf("FAIL: wset->number != wset_test->number\t%d != %d\n", wset->number, wset_test->number);
        }

        Window* windows = wset->_;
        Window* windows_test = wset_test->_;
        for (int wn = 0; wn < wset->number; wn++) {
            const int S = windows_test[wn].metrics.number * sizeof(WindowMetricSet);
            int cmp = memcmp(windows[wn].metrics._, windows_test[wn].metrics._, S);
            eq += cmp;
            if (cmp) {
                printf("FAIL(cmp=%5d): (%6d, %6d, %6d)\n", cmp, capture_index, ws, wn);
                tester_comparer(capture_index, wn, ws, windows[wn].metrics, windows_test[wn].metrics);
            }
        }
    }

    if (!eq) printf("Capture %d compare test success!\n", capture_index);

    return eq;
}