#include "parameters.h"
#include "persister.h"

#include <stdio.h>

void parameters_generate(char* root_dir, PSets* pis_ptr, int* n_psets_ptr) {
    PSets pis;

    char __path[150];
    snprintf(__path, 150, "%s/parameters.bin", root_dir);

    {
        int ret = persister_read_parameters(__path, pis_ptr, n_psets_ptr);

        if (ret == 1) {
            return;
        }
    }

    NN nn[] = { NN_NONE, NN_TLD, NN_ICANN, NN_PRIVATE };

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

    pis = calloc(n_psets, sizeof(PSet));

    *pis_ptr = pis;
    *n_psets_ptr = n_psets;

    int i = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i4 = 0; i4 < count_iv; ++i4) {
                    pis[i].infinite_values = infinitevalues[i4];
                    pis[i].nn = nn[i0];
                    pis[i].whitelisting = whitelisting[i1];
                    pis[i].windowing = windowing_types[i2];
                    pis[i].id = i;

                    ++i;
                }
            }
        }
    }

    {
        int ret = persister_write_parameters(__path, *pis_ptr, *n_psets_ptr);

        if (!ret) printf("Pi-s write error\n");
    }
}
