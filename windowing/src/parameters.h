
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include "common.h"

typedef struct PSet {
    int32_t id;
    
    InfiniteValues infinite_values;
    NN nn;
    Whitelisting whitelisting;
    WindowingType windowing;
    int32_t wsize;
    char digest[SHA256_DIGEST_LENGTH * 2];
} PSet;

typedef struct PSetGenerator {
    int32_t n_wsize;
    int32_t* wsize;

    int32_t n_whitelisting;
    Whitelisting* whitelisting;

    int32_t n_windowing;
    WindowingType* windowing;

    int32_t n_infinitevalues;
    InfiniteValues* infinitevalues;

    int32_t n_nn;
    NN* nn;
} PSetGenerator;

MAKEMANY(PSet);
MAKEMANYCONST(PSet);

void parameters_hash(PSet*);

void parameters_print(PSet* pset);

MANY(PSet) parameters_generate(TCPC(PSetGenerator));


#endif