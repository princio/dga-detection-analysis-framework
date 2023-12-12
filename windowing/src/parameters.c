#include "parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

MANY(PSet) parameters_generate(TCPC(PSetGenerator) psetgenerator) {
    MANY(PSet) psets;

    size_t count_ninf = psetgenerator->n_ninf;
    size_t count_pinf = psetgenerator->n_pinf;
    size_t count_nn = psetgenerator->n_nn;
    size_t count_wl_rank = psetgenerator->n_wl_rank;
    size_t count_wl_value = psetgenerator->n_wl_value;
    size_t count_windowing = psetgenerator->n_windowing;
    size_t count_nx_epsilon_increment = psetgenerator->n_nx_epsilon_increment;

    const size_t n_psets = count_ninf * count_pinf * count_nn * count_wl_rank * count_wl_value * count_windowing * count_nx_epsilon_increment;

    INITMANY(psets, n_psets, PSet);

    size_t i = 0;
    for (size_t i_ninf = 0; i_ninf < count_ninf; ++i_ninf) {
        for (size_t i_pinf = 0; i_pinf < count_pinf; ++i_pinf) {
            for (size_t i_nn = 0; i_nn < count_nn; ++i_nn) {
                for (size_t i_wl_rank = 0; i_wl_rank < count_wl_rank; ++i_wl_rank) {
                    for (size_t i_wl_value = 0; i_wl_value < count_wl_value; ++i_wl_value) {
                        for (size_t i_windowing = 0; i_windowing < count_windowing; ++i_windowing) {
                            for (size_t i_nx_epsilon_increment = 0; i_nx_epsilon_increment < count_nx_epsilon_increment; ++i_nx_epsilon_increment) {
                                PSet* pset = &psets._[i];

                                memset(pset, 0, sizeof(PSet));

                                pset->index = i;
                                
                                pset->ninf = psetgenerator->ninf[i_ninf];
                                pset->pinf = psetgenerator->pinf[i_pinf];
                                pset->nn = psetgenerator->nn[i_nn];
                                pset->wl_rank = psetgenerator->wl_rank[i_wl_rank];
                                pset->wl_value = psetgenerator->wl_value[i_wl_value];
                                pset->windowing = psetgenerator->windowing[i_windowing];
                                pset->nx_epsilon_increment = psetgenerator->nx_epsilon_increment[i_nx_epsilon_increment];

                                ++i;
                            }
                        }
                    }
                }
            }
        }
    }

    return psets;
}

void parameters_generate_free(PSetGenerator* psetgenerator) {
    free(psetgenerator->ninf);
    free(psetgenerator->pinf);
    free(psetgenerator->nn);
    free(psetgenerator->wl_rank);
    free(psetgenerator->wl_value);
    free(psetgenerator->windowing);
    free(psetgenerator->nx_epsilon_increment);
    free(psetgenerator);
}
