
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


void windowing_alloc(Windowed* windowed) {
    Windows* windows = &windowed->windows;

    int32_t n_windows = N_WINDOWS(windowed->source->fnreq_max, windowed->windowing.wsize.value);

    windows->number = n_windows;
    windows->_ = calloc(n_windows, sizeof(Window));

    for (IDX window_idx = 0; window_idx < n_windows; ++window_idx) {

        windows->_[window_idx].wsize_id = windowed->windowing.wsize.id;
        windows->_[window_idx].source_id = windowed->source->id;
        windows->_[window_idx].window_id = window_idx;

        windows->_[window_idx].class = windowed->source->class;
    }

}


void windowing_captures_init(Sources sources, Windowing* windowing) {
    const int32_t wsize = windowing->wsize.value;

    int32_t n_windows = 0;
    for (int capture_index = 0; capture_index < sources.number; capture_index++) {
        n_windows += N_WINDOWS(sources._[capture_index].fnreq_max, wsize);
    }

    windowing->windows.number = n_windows;
    windowing->windows._ = calloc(n_windows, sizeof(Window));

    int32_t n_window = 0;
    for (int source_idx = 0; source_idx < sources.number; source_idx++) {
        SourcePtr source = &sources._[source_idx];

        const int32_t n_windows = N_WINDOWS(source->fnreq_max, wsize);

        for (int source_window_idx = 0; source_window_idx < n_windows; source_window_idx++) {
            Window *window = &windowing->windows._[n_window++];

            window->class = source->class;

            window->source_idx = source_idx;
            window->window_idx = source_window_idx;
            window->pi_id = windowing->pset.id;
            window->wsize_idx = windowing->wsize.id;
        }
    }
}

/*
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
*/



WindowingPtr windowing_run(char* rootpath, char* name, Sources sources, WSizes wsizes, PSets psets) {
    Windowings windowings;

    windowings.number = wsizes.number * psets.number;
    windowings._ = calloc(windowings.number, sizeof(Windowing));

    if(!rootpath) {
        sprintf(rootpath, "/home/princio/Desktop/exps/");
    }
    
    printf("Sources #%d\n", sources.number);
    printf("WSizes #%d\n", wsizes.number);
    printf("Parameters #%d\n", psets.number);

    int32_t windowing_idx = 0;
    for (int32_t s = 0; s < sources.number; s++) {
        for (int32_t w = 0; w < wsizes.number; w++) {
            for (int32_t p = 0; p < psets.number; p++) {
                Windowing* windowing = &windowings._[windowing_idx];
                windowing->id = windowing_idx;
                windowing->wsize = wsizes._[w];
                windowing->pset = &psets._[p];

                ++windowing_idx;
            }
        }
    }


    for (int32_t s = 0; s < sources.number; s++) {
        for (int32_t w = 0; w < windowings.number; w++) {
            Windowing* windowing = &windowings._[windowing_idx];
            windowing->id = windowing_idx;
            windowing->wsize = wsizes._[w];
            windowing->pset = &psets._[p];

            ++windowing_idx;
        }
    }
    
    sources.number = 5; // DEBUG

    for (int32_t i = 0; i < sources.number; i++) {
        sources._[i].fetch(windowings, source_idx);
    }

    // if (0) {
    //     tester_init();

    //     for (int i = 0; i < sources.number; i++) {
    //         printf("%d/%d [%d]\n", i + 1, sources.number, sources._[i].class);
    //         if (persister_read__capturewsets(windowing_ptr, i)) {
    //             for (int try = 0; try < 2; try++) {
    //                 trys = try;
    //                 // windowings[try]->captures._[i].fetch(windowings[try], i);
    //             }
    //             windowing_compare(windowings[0], windowings[1], i);
    //             persister_write__capturewsets(windowing_ptr, i);
    //         }
    //     }
    // }

    // for (int i = 0; i < sources.number; i++) {
    //     printf("%d/%d [%d]\n", i + 1, sources.number, sources._[i].class);
    //     sources._[i].fetch(sources._[i], windowings);
    // }


    // // for (int32_t i = 0; i < windowing_ptr->captures.number; ++i) {
    // //     for (int32_t w = 0; w < windowing_ptr->wsizes.number; ++w) {
    // //         for (int32_t j = 0; j < windowing_ptr->captures_wsets[i][w].number; ++j) {
    // //             for (int32_t m = 0; m < windowing_ptr->psets.number; ++m) {
    // //                 if (!windowing_ptr->captures_wsets[i][w]._[j].metrics._[m].logit)
    // //                 printf("%d,%d,%d,%d\t%f\n", i, w, j, m, windowing_ptr->captures_wsets[i][w]._[j].metrics._[m].logit);
    // //             }
    // //         }
    // //     }
    // // }

    // // exit(0);

    // printf("\nWe are ready for testing!\n");

    // return windowing_ptr;
}

WindowingPtr windowing_load(Experiment* experiment) {
    Windowing* windowing_ptr = calloc(1, sizeof(Windowing));

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
        printf("%d/%d [%d]\n", i + 1, windowing_ptr->captures.number, windowing_ptr->captures._[i].class);
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

    for (int i = 0; i < windowing_ptr->captures.number; i++) {
        printf("%d/%d [%d]\n", i + 1, windowing_ptr->captures.number, windowing_ptr->captures._[i].class);
        save_done = persister_write__capturewsets(windowing_ptr, i);
        if (save_done) {
            printf("Failed to save \"capturewsets\" of %d.\n", i);
            return -1;
        }
    }

    printf("\nWindowing %s saved!\n", windowing_ptr->name);

    return 0;
}
