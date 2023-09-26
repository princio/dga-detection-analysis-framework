#include "parameters.h"

#include <stdio.h>
#include <stdlib.h>

void parameters_generate(WindowingPtr windowing) {
    PSets *psets = &windowing->psets;

    NN nn[] = { NN_NONE };//, NN_TLD, NN_ICANN, NN_PRIVATE };

    Whitelisting whitelisting[] = {
        { .rank = 0, .value = 0 },
        { .rank = 1000, .value = -50 },
        { .rank = 100000, .value = -50 },
        { .rank = 1000000, .value = -10 },  
        { .rank = 1000000, .value = -50 } 
    };

    WindowingType windowing_types[] = {
        WINDOWING_Q,
        WINDOWING_R,
        WINDOWING_QR
    };

    InfiniteValues infinitevalues[] = {
        { .ninf = -20, .pinf = 20 }
    };

    size_t count_nn = sizeof(nn) / sizeof(NN);
    size_t count_wl = sizeof(whitelisting)/sizeof(Whitelisting);
    size_t count_wt = sizeof(windowing_types)/sizeof(WindowingType);
    size_t count_iv = sizeof(infinitevalues)/sizeof(InfiniteValues);

    const int n_psets = count_nn * count_wl * count_wt * count_iv;

    int _n_pis = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i4 = 0; i4 < count_iv; ++i4) {
                    ++_n_pis;
                }
            }
        }
    }

    if (n_psets != _n_pis) {
        fprintf(stderr, "errore calcolo n_psets [n_psets=%d] != [%d]", n_psets, _n_pis);
    }

    printf("NN: %ld\n", count_nn);
    printf("WL: %ld\n", count_wl);
    printf("WT: %ld\n", count_wt);
    printf("IV: %ld\n", count_iv);
    printf("Number of parameters: %d\n", n_psets);

    psets->number = n_psets;
    psets->_ = calloc(n_psets, sizeof(PSet));

    int i = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i4 = 0; i4 < count_iv; ++i4) {
                    psets->_[i].infinite_values = infinitevalues[i4];
                    psets->_[i].nn = nn[i0];
                    psets->_[i].whitelisting.rank = whitelisting[i1].rank;
                    psets->_[i].whitelisting.value = whitelisting[i1].value;
                    psets->_[i].windowing = windowing_types[i2];
                    psets->_[i].id = i;

                    ++i;
                }
            }
        }
    }
}
