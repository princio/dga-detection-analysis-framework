
#include "dataset.h"

#include "windows.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


void dataset_rwindows(Dataset* ds, DGAMANY(RWindow) dsrwindows, int32_t shuffle) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        rwindows_from(ds->windows[cl], &dsrwindows[cl]);

        if (shuffle) {
            rwindows_shuffle(&dsrwindows[cl]);
        }
    }
}
