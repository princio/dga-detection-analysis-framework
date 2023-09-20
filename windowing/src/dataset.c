#include "dataset.h"

#include "persister.h"

#include <assert.h>
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>


void _extract_wd(WindowingRefSet* wrs_class, WindowingRefSet* wrs_train, WindowingRefSet* wrs_test) {
    int total = wrs_class->n_windows;
    int extracted[total]; // 1 means extracted, 0 means not
    int i;

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
            wrs_toextract->windows[i++] = wrs_class->windows[dado];
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
                wrs_tofill->windows[i++] = wrs_class->windows[j];
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

void dataset_fill(WindowingPtr windowing, Dataset dt[]) {

    int32_t n_windows_all = 0;
    int32_t n_windows_classes[windowing->wsizes.number][N_CLASSES];
    memset(n_windows_classes, 0, sizeof(int32_t) * windowing->wsizes.number * N_CLASSES);

    for (int c = 0; c < windowing->captures.number; ++c) {
        CapturePtr capture = &windowing->captures._[c];
        for (int w = 0; w < windowing->wsizes.number; ++w) {
            WSetPtr capture_wsetset = &windowing->captures_wsets[c][w];
            
            n_windows_classes[w][capture->class] += capture_wsetset->n_windows;
            n_windows_all += capture_wsetset->n_windows;
        }
    }



    for (int w = 0; w < windowing->wsizes.number; ++w) {
        dt[w].wsize = windowing->wsizes._[w];
        dt[w].total.n_windows = n_windows_all;
        dt[w].total.windows = calloc(n_windows_all, sizeof(Window*));
        for (int cl = 0; cl < N_CLASSES; ++cl) {
            dt[w].wsize = windowing->wsizes._[w];
            dt[w].classes[cl].n_windows = n_windows_classes[cl];
            dt[w].classes[cl].windows = calloc(n_windows_classes[cl], sizeof(Window*));
        }
    }


    int32_t cursor_all = 0;
    int32_t cursor_classes[windowing->wsizes.number][N_CLASSES];
    memset(cursor_classes, 0, sizeof(int32_t) * windowing->wsizes.number * N_CLASSES);
    for (int w = 0; w < windowing->wsizes.number; ++w) {
        for (int c = 0; c < windowing->wsizes.number; ++c) {
            WSetPtr capture_wsetset = &windowing->captures_wsets[c][w];
            Class class = &windowing->captures._[c].class;
            WindowingRefSet* wrf = &dt[w].classes[class];
            for (int i = 0; i < capture_wsetset->n_windows; ++i) {
                wrf->windows[cursor_classes[w][class]++] = &capture_wsetset->windows[i];
                dt[w].total.windows[cursor_all++] = &capture_wsetset->windows[i];
            }
        }
    }
}

void dataset_traintest(DatasetPtr dt, DatasetTrainTestPtr dt_tt, double percentage_split) {
    assert(percentage_split > 0 & percentage_split <= 1);

    dt_tt->percentage_split = percentage_split;

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        WindowingRefSet* wrs_train = &dt_tt->train[cl];
        WindowingRefSet* wrs_test = &dt_tt->test[cl];

        wrs_train->n_windows = floor(dt->classes[cl].n_windows * percentage_split);
        wrs_train->windows = calloc(wrs_train->n_windows, sizeof(Window*));

        wrs_test->n_windows = dt->classes[cl].n_windows - wrs_train->n_windows;
        wrs_test->windows = calloc(wrs_test->n_windows, sizeof(Window*));


        _extract_wd(&dt->classes[cl], wrs_train, wrs_test);
    }
}


void dataset_traintest_cm(PSets* psets, DatasetTrainTestPtr dt_tt, double *th, int (*cm)[N_CLASSES][2]) {

    for (int cl = 0; cl < N_CLASSES; ++cl) {
        WindowingRefSet* wrs = &dt_tt->train[cl];

        for (int i = 0; i < wrs->n_windows; ++i) {
            Window* window = wrs->windows[i];

            for (int m = 0; m < psets->number; ++m) {
                int32_t class_predicted;
                int32_t is_true;

                class_predicted = window->metrics._[m].logit >= th[m];

                is_true = (window->class / N_CLASSES) == class_predicted; // binary trick

                cm[m][cl][is_true] += 1;
            }
        }
    }
}