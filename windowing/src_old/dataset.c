#include "dataset.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void print_cm(const int N_PSETS, CM* cm) {
    for (int m = 0; m < N_PSETS; ++m) {
        printf(
            "\n| %5d, %5d |\n| %5d, %5d \n",
            cm->single[0].trues,
            cm->single[0].falses,
            cm->single[1].falses,
            cm->single[1].trues
        );
    }
}

void _extract_wd(WSetRef* wrs_class, WSetRef* wrs_train, WSetRef* wrs_test) {
    int total = wrs_class->number;
    int extracted[total]; // 1 means extracted, 0 means not
    memset(extracted, 0, sizeof(int) * total);

    WSetRef* wrs_toextract;
    WSetRef* wrs_tofill;
    if (wrs_train->number > wrs_test->number) {
        wrs_toextract = wrs_train;
        wrs_tofill = wrs_test;
    } else {
        wrs_toextract = wrs_test;
        wrs_tofill = wrs_train;
    }

    {
        srand(time(NULL));
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
            printf("Guard error [%d >= %d] %d\n", guard_counter, guard, wrs_class->number);
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

    int32_t n_windows_all[windowing->wsizes.number];
    int32_t n_windows_classes[windowing->wsizes.number][N_CLASSES];
    memset(n_windows_all, 0, sizeof(int32_t) * windowing->wsizes.number);
    memset(n_windows_classes, 0, sizeof(int32_t) * windowing->wsizes.number * N_CLASSES);

    for (int c = 0; c < windowing->captures.number; ++c) {
        SourcePtr capture = &windowing->captures._[c];
        for (int w = 0; w < windowing->wsizes.number; ++w) {
            WSetPtr capture_wsetset = &windowing->captures_wsets[c][w];
            
            n_windows_classes[w][capture->class] += capture_wsetset->number;
            n_windows_all[w] += capture_wsetset->number;
        }
    }

    // for (int w = 0; w < windowing->wsizes.number; ++w) {
    //     printf("n_windows_all  [%4d]      \t%d\n", w, n_windows_all[w]);
    //     for (int cl = 0; cl < N_CLASSES; ++cl) {
    //         printf("n_windows_class[%4d][%4d]\t%d\n", w, cl, n_windows_classes[w][cl]);
    //     }
    // }

    for (int w = 0; w < windowing->wsizes.number; ++w) {
        dt[w].wsize = windowing->wsizes._[w];
        dt[w].windows_all.number = n_windows_all[w];
        dt[w].windows_all._ = calloc(n_windows_all[w], sizeof(Window*));
        for (int cl = 0; cl < N_CLASSES; ++cl) {
            dt[w].wsize = windowing->wsizes._[w];
            dt[w].windows[cl].number = n_windows_classes[w][cl];
            dt[w].windows[cl]._ = calloc(n_windows_classes[w][cl], sizeof(Window*));
        }
    }


    int32_t cursor_all[windowing->wsizes.number];
    int32_t cursor_classes[windowing->wsizes.number][N_CLASSES];
    memset(cursor_all, 0, sizeof(int32_t) * windowing->wsizes.number);
    memset(cursor_classes, 0, sizeof(int32_t) * windowing->wsizes.number * N_CLASSES);
    for (int w = 0; w < windowing->wsizes.number; ++w) {
        for (int c = 0; c < windowing->captures.number; ++c) {
            WSetPtr capture_wset = &windowing->captures_wsets[c][w];
            Class class = windowing->captures._[c].class;
            WSetRef* wrf = &dt[w].windows[class];
            for (int i = 0; i < capture_wset->number; ++i) {
                assert(cursor_classes[w][class] < wrf->number);
                assert(cursor_all[w] < dt[w].windows_all.number);
                wrf->_[cursor_classes[w][class]++] = &capture_wset->_[i];
                dt[w].windows_all._[cursor_all[w]++] = &capture_wset->_[i];
            }
        }
    }
}

void dataset_traintestsplit(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split) {
    assert((percentage_split > 0) & (percentage_split < 1));

    dt_tt->percentage_split = percentage_split;

    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        WSetRef* wrs_train = &dt_tt->train[cl];
        WSetRef* wrs_test = &dt_tt->test[cl];

        if (dt->windows[cl].number == 0) {
            // printf("cl=%d\t%d\t%d\t%d\n", cl, dt->windows[cl].number, wrs_train->number, wrs_test->number);
            wrs_train->number = 0;
            wrs_train->_ = NULL;
            wrs_test->number = 0;
            wrs_test->_ = NULL;
            continue;
        }

        wrs_train->number = floor(dt->windows[cl].number * percentage_split);
        wrs_train->number = wrs_train->number == 0 ? 1 : wrs_train->number;
        wrs_train->_ = calloc(wrs_train->number, sizeof(Window*));

        wrs_test->number = dt->windows[cl].number - wrs_train->number;
        wrs_test->_ = calloc(wrs_test->number, sizeof(Window*));

        // printf("dt->windows[cl=%d].number\t%d\n", cl, dt->windows[cl].number);

        _extract_wd(&dt->windows[cl], wrs_train, wrs_test);
    }
}

void logit_range(double min, double max, WSetRef* ref) {
    for (int32_t i = 0; i < ref->number; ++i) {
        ref->_[i]->metrics[]
    }
}

void dataset_traintestsplit_cm(int32_t wsize, PSets* psets, DatasetTrainTestPtr dt_tt) {

    for (int m = 0; m < psets->number; ++m) {
        double th = -1 * DBL_MAX;
        WSetRef* ws_ni = &dt_tt->train[CLASS__NOT_INFECTED];

        for (int i = 0; i < ws_ni->number; i++) {
            double logit = ws_ni->_[i]->metrics._[m].logit;
            if (th < logit) th = logit;
        }

        dt_tt->ths._[m] = th;


    }   

    for (int cl = 0; cl < N_CLASSES; ++cl) {
        if (dt_tt->test->number == 0) continue;

        WSetRef* wrs = &dt_tt->test[cl];

        for (int i = 0; i < wrs->number; ++i) {
            Window* window = wrs->_[i];

            for (int m = 0; m < psets->number; ++m) {
                int32_t prediction; // 1 or 0
                int32_t binary_class;  // 1 or 0

                prediction = window->metrics._[m].logit >= dt_tt->ths._[m];
                binary_class = window->class == CLASS__NOT_INFECTED ? 0 : 1;

                // printf("%d,%d,%d,%s,%5.1f\t%5.1f\n", window->class, binary_class, prediction, prediction == binary_class ? "Goot" : "Wrong", window->metrics._[m].logit, dt_tt->ths._[m]);
                dt_tt->cms._[m].wsize = wsize;
                dt_tt->cms._[m].pset = &psets->_[m];
                
                if (prediction == binary_class) {
                    dt_tt->cms._[m].single[binary_class].trues += 1;
                    dt_tt->cms._[m].multi[window->class].trues += 1;
                } else {
                    dt_tt->cms._[m].single[binary_class].falses += 1;
                    dt_tt->cms._[m].multi[window->class].falses += 1;
                }
            }
        }
    }
}