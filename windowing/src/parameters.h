
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include "common.h"

#include "io.h"

typedef struct PSet {
    size_t id;
    
    double ninf;
    double pinf;

    NN nn;
    
    size_t wl_rank;
    double wl_value;

    WindowingType windowing;

    float nx_epsilon_increment;
} PSet;

typedef PSet* RPSet;

typedef struct PSetGenerator {
    size_t n_ninf;
    double* ninf;

    size_t n_pinf;
    double* pinf;

    size_t n_wl_rank;
    size_t* wl_rank;

    size_t n_wl_value;
    double* wl_value;

    size_t n_windowing;
    WindowingType* windowing;

    int32_t n_nn;
    NN* nn;

    size_t n_nx_epsilon_increment;
    float* nx_epsilon_increment;
} PSetGenerator;

MAKEMANY(PSet);

void parameters_hash(PSet*);

void parameters_print(PSet* pset);

MANY(PSet) parameters_generate(TCPC(PSetGenerator));
void parameters_generate_free(PSetGenerator*);

#endif