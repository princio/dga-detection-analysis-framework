
#include "dataset.h"

#include "windows.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


int _ths_exist(int n, double ths[n], int th) {
    for (int32_t i = 0; i < n; i++) {
        if (ths[i] == th) return 1;
    }
    return 0;
}


void dataset_rwindows(Dataset* ds, DatasetRWindows dsrwindows, int32_t shuffle) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        rwindows_from(ds->windows[cl], &dsrwindows[cl]);

        if (shuffle) {
            rwindows_shuffle(&dsrwindows[cl]);
        }
    }
}


void dataset_rwindows_ths(DatasetRWindows dsrwindows, Ths* ths) {
    int max = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        max += dsrwindows[cl].number;
    }

    double* ths_tmp = calloc(max, sizeof(double));

    int n = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        for (int32_t w = 0; w < dsrwindows[cl].number; w++) {
            int logit = floor(dsrwindows[cl]._[w]->logit);
            int exists = _ths_exist(n, ths_tmp, logit);
            if(!exists) {
                ths_tmp[n++] = logit;
            }
        }
    }

    ths->number = n;
    ths->_ = calloc(n, sizeof(double));
    memcpy(ths->_, ths_tmp, sizeof(double) * n);

    free(ths_tmp);
}