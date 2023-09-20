#include "windows.h"

#include "persister.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>


void _extract_wd(Dataset* dt, DatasetTrainTest* dt_tt) {
    int total = dt->total.n_windows;
    int extracted[total]; // 1 means extracted, 0 means not
    int i;

    dt_tt->tr

    WindowingRefSet* wrs_toextract = (wrs_train->n_windows > wrs_test->n_windows) ? wrs_train : wrs_test;
    WindowingRefSet* wrs_tofill = (wrs_train->n_windows > wrs_test->n_windows) ? wrs_test : wrs_train;

    memset(extracted, 0, sizeof(int) * total);
    srand(time(NULL));

    {
        int guard = total * 100;
        int guard_counter = 0;
        int i = 0;
        while (++guard_counter < guard && i < wrs_toextract->n_windows) {
            int dado = rand() % total;

            if (extracted[dado]) continue;

            extracted[dado] = 1;
            wrs_toextract->windows[i++] = wrs_total->windows[dado];
            if (wrs_toextract->n_windows < i) {
                printf("Error wrs_toextract->n_windows <= i (%d <= %d)\n", wrs_toextract->n_windows , i);
            }
        }

        if (i != wrs_toextract->n_windows) {
            printf("Error %d != %d\n", i, wrs_toextract->n_windows);
        }

        if (guard_counter >= guard) {
            printf("Guard error\n");
        }
    }

    {
        int i = 0;
        for (int j = 0; j < total; ++j) {
            if (0 == extracted[j]) {
                wrs_tofill->windows[i++] = wrs_total->windows[j];
                if (wrs_tofill->n_windows < i) {
                    printf("Error wrs_tofill->n_windows <= i (%d <= %d)\n", wrs_tofill->n_windows , i);
                }
            }
        }

        if (i != wrs_tofill->n_windows) {
            printf("Error %d != %d\n", i, wrs_tofill->n_windows);
        }
    }
}

void dataset_addcapture(char* root_dir, WindowingPtr windowing, CapturePtr capture) {
    int32_t capture_index = windowing->n_captures;

    windowing->captures[capture_index] = capture;
    windowing->n_captures += 1;
}

void dataset_fill(Windowing* windowing, Dataset dt[windowing->n_wsizes]) {

    int32_t n_windows_all = 0;
    int32_t n_windows_classes[windowing->n_wsizes][N_CLASSES];
    memset(n_windows_classes, 0, sizeof(int32_t) * windowing->n_wsizes * N_CLASSES);

    for (int c = 0; c < windowing->n_captures; ++c) {
        CapturePtr capture = &windowing->captures[c];
        for (int w = 0; w < windowing->n_wsizes; ++w) {
            WindowingSetPtr capture_windowingset = &windowing->captures_windowings[c][w];
            
            n_windows_classes[w][capture->class] += capture_windowingset->n_windows;
            n_windows_all += capture_windowingset->n_windows;
        }
    }



    for (int w = 0; w < windowing->n_wsizes; ++w) {
        dt[w].wsize = windowing->wsizes[w];
        dt[w].total.n_windows = n_windows_all;
        dt[w].total.windows = calloc(n_windows_all, sizeof(Window*));
        for (int cl = 0; cl < N_CLASSES; ++cl) {
            dt[w].wsize = windowing->wsizes[w];
            dt[w].classes[cl].n_windows = n_windows_classes[cl];
            dt[w].classes[cl].windows = calloc(n_windows_classes[cl], sizeof(Window*));
        }
    }


    int32_t cursor_all = 0;
    int32_t cursor_classes[windowing->n_wsizes][N_CLASSES];
    memset(cursor_classes, 0, sizeof(int32_t) * windowing->n_wsizes * N_CLASSES);
    for (int w = 0; w < windowing->n_wsizes; ++w) {
        for (int c = 0; c < windowing->n_captures; ++c) {
            WindowingSetPtr capture_windowingset = &windowing->captures_windowings[c][w];
            Class class = &windowing->captures[c]->class;
            WindowingRefSet* wrf = &dt[w].classes[class];
            for (int i = 0; i < capture_windowingset->n_windows; ++i) {
                wrf->windows[cursor_classes[w][class]++] = &capture_windowingset->windows[i];
                dt[w].total.windows[cursor_all++] = &capture_windowingset->windows[i];
            }
        }
    }
}

void dataset_traintest(DatasetPtr dt, DatasetSplitPtr dtsplit) {
    for (int32_t cl = 0; cl < N_CLASSES; cl++)
    {
        _extract_wd(&dt->classes[cl], &dtsplit->train[cl], &dtsplit->train[cl], dtsplit->percentage_split);
    }
    
}