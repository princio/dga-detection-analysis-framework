#include "parameters.h"

#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

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

MANY(PSet) parameters_generate(TCPC(PSetGenerator) psetgenerator) {
    size_t count_ws = psetgenerator->n_wsize;
    size_t count_nn = psetgenerator->n_nn;
    size_t count_wl = psetgenerator->n_whitelisting;
    size_t count_wt = psetgenerator->n_windowing;
    size_t count_iv = psetgenerator->n_infinitevalues;

    const int n_psets = count_ws * count_nn * count_wl * count_wt * count_iv;

    MANY(PSet) psets = {
        .number = n_psets,
        ._ = calloc(n_psets, sizeof(PSet))
    };

    int i = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i4 = 0; i4 < count_iv; ++i4) {
                    for (size_t i5 = 0; i5 < count_ws; ++i5) {
                        PSet* pset = &psets._[i];

                        pset->id = i;
                        
                        pset->wsize = psetgenerator->wsize[i5];
                        pset->infinite_values = psetgenerator->infinitevalues[i4];
                        pset->nn = psetgenerator->nn[i0];
                        pset->whitelisting.rank = psetgenerator->whitelisting[i1].rank;
                        pset->whitelisting.value = psetgenerator->whitelisting[i1].value;
                        pset->windowing = psetgenerator->windowing[i2];

                        ++i;
                    }
                }
            }
        }
    }

    return psets;
}

void parameters_io(IOReadWrite rw, FILE* file, void* obj) {
    PSet *pset = obj;

    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(pset->id);
    FRW(pset->infinite_values);
    FRW(pset->nn);
    FRW(pset->whitelisting);
    FRW(pset->windowing);
    FRW(pset->wsize);
}

void parameters_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    memset(objid, 0, IO_OBJECTID_LENGTH);
    io_subdigest(obj, sizeof(PSet), objid);
}

