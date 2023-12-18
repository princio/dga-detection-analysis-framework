#include "parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>


MANY(PSet) parameters_fields2psets(TCPC(PSetByField) psetbyfield) {
    MANY(PSet) psets;

    const size_t n_psets = psetbyfield->ninf.number * psetbyfield->pinf.number * psetbyfield->nn.number * psetbyfield->wl_rank.number * psetbyfield->wl_value.number * psetbyfield->windowing.number * psetbyfield->nx_epsilon_increment.number;

    INITMANY(psets, n_psets, PSet);

    size_t i = 0;
    for (size_t i_ninf = 0; i_ninf < psetbyfield->ninf.number; ++i_ninf) {
        for (size_t i_pinf = 0; i_pinf < psetbyfield->pinf.number; ++i_pinf) {
            for (size_t i_nn = 0; i_nn < psetbyfield->nn.number; ++i_nn) {
                for (size_t i_wl_rank = 0; i_wl_rank < psetbyfield->wl_rank.number; ++i_wl_rank) {
                    for (size_t i_wl_value = 0; i_wl_value < psetbyfield->wl_value.number; ++i_wl_value) {
                        for (size_t i_windowing = 0; i_windowing < psetbyfield->windowing.number; ++i_windowing) {
                            for (size_t i_nx_epsilon_increment = 0; i_nx_epsilon_increment < psetbyfield->nx_epsilon_increment.number; ++i_nx_epsilon_increment) {
                                PSet* pset = &psets._[i];

                                memset(pset, 0, sizeof(PSet));

                                pset->index = i;
                                
                                pset->ninf = psetbyfield->ninf._[i_ninf];
                                pset->pinf = psetbyfield->pinf._[i_pinf];
                                pset->nn = psetbyfield->nn._[i_nn];
                                pset->wl_rank = psetbyfield->wl_rank._[i_wl_rank];
                                pset->wl_value = psetbyfield->wl_value._[i_wl_value];
                                pset->windowing = psetbyfield->windowing._[i_windowing];
                                pset->nx_epsilon_increment = psetbyfield->nx_epsilon_increment._[i_nx_epsilon_increment];

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

PSetByField parameters_psets2byfields(MANY(PSet)* psets) {
    PSetByField sp;
    
    #define pc_init(NAME)\
    INITMANY(sp.NAME, psets->number, NAME ## _t);\
    sp.NAME.number = 0;

    #define pc_search(NAME) {\
            int found = 0;\
            for (size_t idx_ ## NAME = 0; idx_ ## NAME < sp.NAME.number; idx_ ## NAME++) {\
                if (psets->_[idxpset].NAME ==  sp.NAME._[idx_ ## NAME]) {\
                    found = 1;\
                }\
            }\
            if (!found) {\
                NAME ## _t NAME ## _value = psets->_[idxpset].NAME;\
                printf("%s: %"PSET_FMT(NAME)"\n", #NAME, PSET_FMT_PRE(NAME));\
                sp.NAME._[sp.NAME.number++] = NAME ## _value;\
            }\
        }

    multi_psetitem(pc_init);

    for (size_t idxpset = 0; idxpset < psets->number; idxpset++) {
        pc_search(ninf);
        multi_psetitem(pc_search);
    }

    #undef pc_init
    #undef pc_search

    return sp;
}

void parameters_fields_free(PSetByField* psetbyfield) {
    FREEMANY(psetbyfield->ninf);
    FREEMANY(psetbyfield->pinf);
    FREEMANY(psetbyfield->nn);
    FREEMANY(psetbyfield->wl_rank);
    FREEMANY(psetbyfield->wl_value);
    FREEMANY(psetbyfield->windowing);
    FREEMANY(psetbyfield->nx_epsilon_increment);
}
