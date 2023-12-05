#include "parameters.h"

#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

MANY(PSet) parameters_generate(TCPC(PSetGenerator) psetgenerator) {
    MANY(PSet) psets;

    size_t count_nn = psetgenerator->n_nn;
    size_t count_wl = psetgenerator->n_whitelisting;
    size_t count_wt = psetgenerator->n_windowing;
    size_t count_iv = psetgenerator->n_infinitevalues;
    size_t count_nx = psetgenerator->n_nx;

    const size_t n_psets = count_nn * count_wl * count_wt * count_iv * count_nx;

    INITMANY(psets, n_psets, PSet);

    size_t i = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i4 = 0; i4 < count_iv; ++i4) {
                    for (size_t i5 = 0; i5 < count_iv; ++i5) {
                        PSet* pset = &psets._[i];

                        memset(pset, 0, sizeof(PSet));

                        pset->id = i;
                        
                        pset->infinite_values = psetgenerator->infinitevalues[i4];
                        pset->nn = psetgenerator->nn[i0];
                        pset->whitelisting = psetgenerator->whitelisting[i1];
                        pset->windowing = psetgenerator->windowing[i2];
                        pset->nx_epsilon_increment = psetgenerator->nx_epsilon_increment[i5];

                        ++i;
                    }
                }
            }
        }
    }

    return psets;
}

void parameters_generate_free(PSetGenerator* psetgenerator) {
    free(psetgenerator->nn);
    free(psetgenerator->whitelisting);
    free(psetgenerator->windowing);
    free(psetgenerator->infinitevalues);
    free(psetgenerator->nx_epsilon_increment);
    free(psetgenerator);
}

void parameters_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    char subdigest[IO_SUBDIGEST_LENGTH];
    memset(objid, 0, IO_OBJECTID_LENGTH);
    io_subdigest(obj, sizeof(PSet), subdigest);
    sprintf(objid, "pset_%s", subdigest);
}

