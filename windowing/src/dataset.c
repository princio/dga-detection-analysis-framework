#include "dataset.h"

#include "persister.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


void _extract_wd(WSetRef* wrs_class, WSetRef* wrs_train, WSetRef* wrs_test) {
    int total = wrs_class->number;
    int extracted[total]; // 1 means extracted, 0 means not

    WSetRef* wrs_toextract = (wrs_train->number > wrs_test->number) ? wrs_train : wrs_test;
    WSetRef* wrs_tofill = (wrs_train->number > wrs_test->number) ? wrs_test : wrs_train;

    memset(extracted, 0, sizeof(int) * total);
    srand(time(NULL));

    {
        int guard = total * 100;
        int guard_counter = 0;
        int i = 0;
        while (++guard_counter < guard && i < wrs_toextract->number) {
            int dado = rand() % total;

            if (extracted[dado]) continue;

            extracted[dado] = 1;
            wrs_toextract->_[i++] = wrs_class->_[dado];
            if (wrs_toextract->number < i) {
                printf("Error wrs_toextract->number <= i (%d <= %d)\n", wrs_toextract->number , i);
            }
        }

        if (i != wrs_toextract->number) {
            printf("Error %d != %d\n", i, wrs_toextract->number);
        }

        if (guard_counter >= guard) {
            printf("Guard error\n");
        }
    }

    {
        int i = 0;
        for (int j = 0; j < total; ++j) {
            if (0 == extracted[j]) {
                wrs_tofill->_[i++] = wrs_class->_[j];
                if (wrs_tofill->number < i) {
                    printf("Error wrs_tofill->number <= i (%d <= %d)\n", wrs_tofill->number , i);
                }
            }
        }

        if (i != wrs_tofill->number) {
            printf("Error %d != %d\n", i, wrs_tofill->number);
        }
    }
}

void dataset_fill(WindowingPtr windowing, Dataset* dt) {

    int32_t n_windows_all = 0;
    int32_t n_windows_classes[windowing->wsizes.number][N_CLASSES];
    memset(n_windows_classes, 0, sizeof(int32_t) * windowing->wsizes.number * N_CLASSES);

    for (int c = 0; c < windowing->captures.number; ++c) {
        CapturePtr capture = &windowing->captures._[c];
        for (int w = 0; w < windowing->wsizes.number; ++w) {
            WSetPtr capture_wsetset = &windowing->captures_wsets[c][w];
            
            n_windows_classes[w][capture->class] += capture_wsetset->number;
            n_windows_all += capture_wsetset->number;
        }
    }



    for (int w = 0; w < windowing->wsizes.number; ++w) {
        dt[w].wsize = windowing->wsizes._[w];
        dt[w].total.number = n_windows_all;
        dt[w].total._ = calloc(n_windows_all, sizeof(Window*));
        for (int cl = 0; cl < N_CLASSES; ++cl) {
            dt[w].wsize = windowing->wsizes._[w];
            dt[w].classes[cl].number = n_windows_classes[w][cl];
            dt[w].classes[cl]._ = calloc(n_windows_classes[w][cl], sizeof(Window*));
        }
    }


    int32_t cursor_all = 0;
    int32_t cursor_classes[windowing->wsizes.number][N_CLASSES];
    memset(cursor_classes, 0, sizeof(int32_t) * windowing->wsizes.number * N_CLASSES);
    for (int w = 0; w < windowing->wsizes.number; ++w) {
        for (int c = 0; c < windowing->wsizes.number; ++c) {
            WSetPtr capture_wsetset = &windowing->captures_wsets[c][w];
            Class class = windowing->captures._[c].class;
            WSetRef* wrf = &dt[w].classes[class];
            for (int i = 0; i < capture_wsetset->number; ++i) {
                wrf->_[cursor_classes[w][class]++] = &capture_wsetset->_[i];
                dt[w].total._[cursor_all++] = &capture_wsetset->_[i];
            }
        }
    }
}

void dataset_traintest(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split) {
    assert((percentage_split > 0) & (percentage_split <= 1));

    dt_tt->percentage_split = percentage_split;

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        WSetRef* wrs_train = &dt_tt->train[cl];
        WSetRef* wrs_test = &dt_tt->test[cl];

        wrs_train->number = floor(dt->classes[cl].number * percentage_split);
        wrs_train->_ = calloc(wrs_train->number, sizeof(Window*));

        wrs_test->number = dt->classes[cl].number - wrs_train->number;
        wrs_test->_ = calloc(wrs_test->number, sizeof(Window*));


        _extract_wd(&dt->classes[cl], wrs_train, wrs_test);
    }
}


void dataset_traintest_cm(PSets* psets, DatasetTrainTestPtr dt_tt, double *th, int (*cm)[N_CLASSES][2]) {

    for (int cl = 0; cl < N_CLASSES; ++cl) {
        WSetRef* wrs = &dt_tt->train[cl];

        for (int i = 0; i < wrs->number; ++i) {
            Window* window = wrs->_[i];

            for (int m = 0; m < psets->number; ++m) {
                Class class_predicted;
                int32_t is_true;

                class_predicted = window->metrics._[m].logit >= th[m];

                is_true = ((Class)(window->class / N_CLASSES)) == class_predicted; // binary trick

                cm[m][cl][is_true] += 1;
            }
        }
    }
}