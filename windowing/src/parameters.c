#include "parameters.h"

#include <stdio.h>
#include <stdlib.h>

#include <openssl/sha.h>

void parameters_hash(PSet* pset) {
    uint8_t out[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha;

    SHA256_Init(&sha);
    SHA256_Update(&sha, pset, sizeof(PSet));
    SHA256_Final(out, &sha);

    char hex[16] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        pset->digest[i * 2] = hex[out[i] >> 4];
        pset->digest[i * 2 + 1] = hex[out[i] & 0x0F];
    }
}

void parameters_print(PSet* pset) {
    printf("%8d, ", pset->wsize);
    printf("(%8d, %8g), ", pset->whitelisting.rank, pset->whitelisting.value);
    printf("(%5g, %5g), ", pset->infinite_values.ninf, pset->infinite_values.pinf);
    printf("%8s, ", NN_NAMES[pset->nn]);
    printf("%d\n", pset->windowing);

    // DumpHex(&pset->infinite_values, sizeof(InfiniteValues));
    // DumpHex(&pset->nn, sizeof(NN));
    // DumpHex(&pset->whitelisting, sizeof(Whitelisting));
    // DumpHex(&pset->windowing, sizeof(WindowingType));
    // DumpHex(&pset->wsize, sizeof(int32_t));

    printf("\n");
}

PSets parameters_generate(PSetGenerator* psetgenerator) {
    PSets psets;

    size_t count_ws = psetgenerator->n_wsize;
    size_t count_nn = psetgenerator->n_nn;
    size_t count_wl = psetgenerator->n_whitelisting;
    size_t count_wt = psetgenerator->n_windowing;
    size_t count_iv = psetgenerator->n_infinitevalues;

    const int n_psets = count_ws * count_nn * count_wl * count_wt * count_iv;

    psets.number = n_psets;
    psets._ = calloc(n_psets, sizeof(PSet));

    int i = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i4 = 0; i4 < count_iv; ++i4) {
                    for (size_t i5 = 0; i5 < count_ws; ++i5) {
                        psets._[i].wsize = psetgenerator->wsize[i5];
                        psets._[i].infinite_values = psetgenerator->infinitevalues[i4];
                        psets._[i].nn = psetgenerator->nn[i0];
                        psets._[i].whitelisting.rank = psetgenerator->whitelisting[i1].rank;
                        psets._[i].whitelisting.value = psetgenerator->whitelisting[i1].value;
                        psets._[i].windowing = psetgenerator->windowing[i2];

                        ++i;
                    }
                }
            }
        }
    }

    for (int32_t p = 0; p < n_psets; p++) {
        parameters_hash(&psets._[p]);
    }
    
    return psets;
}
