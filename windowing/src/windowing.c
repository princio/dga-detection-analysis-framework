
#include "windowing.h"

#include "dn.h"
#include "stratosphere.h"
#include "parameters.h"
#include "persister.h"
#include "tester.h"

#include <assert.h>
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

WindowingPtr windowing_run(char* rootpath, char* name, WSizes wsizes, PSetGenerator* psetgenerator) {
    WindowingPtr windowing_ptr = calloc(1, sizeof(Windowing));

    Windowing windowing_test;
    WindowingPtr windowing_ptr_test = &windowing_test;

    if(rootpath) {
        assert(strlen(rootpath) < 200);
        memcpy(windowing_ptr->rootpath, rootpath, strlen(rootpath));
    } else {
        sprintf(windowing_ptr->rootpath, "/home/princio/Desktop/exps/");
    }

    sprintf(windowing_ptr->name, "%s", name);

    windowing_ptr->captures.number = 0;
    windowing_ptr->psets.number = 0;

    windowing_ptr->wsizes.number = wsizes.number;

    for (int i = 0; i < wsizes.number; i++) {
        windowing_ptr->wsizes._[i] = wsizes._[i];
    }
    
    parameters_generate(windowing_ptr, psetgenerator);

    printf("Parameters #%d\n", windowing_ptr->psets.number);

    stratosphere_add_captures(windowing_ptr);

    windowing_test = *windowing_ptr;

    printf("Captures #%d\n", windowing_ptr->captures.number);

    printf("Windows initialization");
    windowing_captures_init(windowing_ptr);
    windowing_captures_init(windowing_ptr_test);

    WindowingPtr windowings[2];
    windowings[0] = windowing_ptr;
    windowings[1] = windowing_ptr_test;

    windowing_ptr->captures.number = 5;

    if (0) {
        tester_init();

        for (int i = 0; i < windowing_ptr->captures.number; i++) {
            printf("%d/%d\n", i + 1, windowing_ptr->captures.number);
            if (persister_read__capturewsets(windowing_ptr, i)) {
                for (int try = 0; try < 2; try++) {
                    trys = try;
                    windowings[try]->captures._[i].fetch(windowings[try], i);
                }
                windowing_compare(windowings[0], windowings[1], i);
                persister_write__capturewsets(windowing_ptr, i);
            }
        }
    }

    for (int i = 0; i < windowing_ptr->captures.number; i++) {
        printf("%d/%d\n", i + 1, windowing_ptr->captures.number);
        windowing_ptr->captures._[i].fetch(windowing_ptr, i);
    }

    printf("\nWe are ready for testing!\n");

    return windowing_ptr;
}

WindowingPtr windowing_load(char* rootpath, char* name) {
    Windowing* windowing_ptr = calloc(1, sizeof(Windowing));

    strcpy(windowing_ptr->name, name);
    strcpy(windowing_ptr->rootpath, rootpath);

    int read_done;
    read_done = persister_read__windowing(windowing_ptr);
    if (read_done) {
        printf("Failed to load \"windowing\".\n");
        return NULL;
    }
    
    read_done = persister_read__psets(windowing_ptr);
    if (read_done) {
        printf("Failed to load \"psets\".\n");
        return NULL;
    }
    
    read_done = persister_read__captures(windowing_ptr);
    if (read_done) {
        printf("Failed to load \"captures\".\n");
        return NULL;
    }

    windowing_captures_init(windowing_ptr);

    for (int i = 0; i < windowing_ptr->captures.number; i++) {
        printf("%d/%d\n", i + 1, windowing_ptr->captures.number);
        read_done = persister_read__capturewsets(windowing_ptr, i);
        if (read_done) {
            printf("Failed to load \"capturewsets\" of %d.\n", i);
            return NULL;
        }
    }

    printf("\nWindowing %s loaded!\n", windowing_ptr->name);

    return windowing_ptr;
}

int windowing_save(WindowingPtr windowing_ptr) {
    int save_done;

    persister_description(windowing_ptr);

    save_done = persister_write__windowing(windowing_ptr);
    if (save_done) {
        printf("Failed to save \"windowing\".\n");
        return -1;
    }
    
    save_done = persister_write__psets(windowing_ptr);
    if (save_done) {
        printf("Failed to save \"psets\".\n");
        return -1;
    }
    
    save_done = persister_write__captures(windowing_ptr);
    if (save_done) {
        printf("Failed to save \"captures\".\n");
        return -1;
    }

    windowing_captures_init(windowing_ptr);

    for (int i = 0; i < windowing_ptr->captures.number; i++) {
        printf("%d/%d\n", i + 1, windowing_ptr->captures.number);
        save_done = persister_write__capturewsets(windowing_ptr, i);
        if (save_done) {
            printf("Failed to save \"capturewsets\" of %d.\n", i);
            return -1;
        }
    }

    printf("\nWindowing %s saved!\n", windowing_ptr->name);

    return 0;
}
