
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include "common.h"

#include "io.h"

typedef struct PSet {
    size_t index;
    
    double ninf;
    double pinf;

    NN nn;
    
    size_t wl_rank;
    double wl_value;

    WindowingType windowing;

    float nx_epsilon_increment;
} PSet;

#define N_PSET_ITEMS 7

typedef PSet* RPSet;

MAKEMANY(PSet);


#define MAKEPSETTYPE(T, A) typedef struct A ## _t { \
    int ignore; \
    T value; \
} A ## _t;

typedef size_t index_t;
typedef double ninf_t;
typedef double pinf_t;
typedef NN nn_t;
typedef size_t wl_rank_t;
typedef double wl_value_t;
typedef WindowingType windowing_t;
typedef float nx_epsilon_increment_t;

MAKEMANY(ninf_t);
MAKEMANY(pinf_t);
MAKEMANY(nn_t);
MAKEMANY(wl_rank_t);
MAKEMANY(wl_value_t);
MAKEMANY(windowing_t);
MAKEMANY(nx_epsilon_increment_t);

typedef struct PSetByField {
    MANY(ninf_t) ninf;
    MANY(pinf_t) pinf;
    MANY(nn_t) nn;
    MANY(wl_rank_t) wl_rank;
    MANY(wl_value_t) wl_value;
    MANY(windowing_t) windowing;
    MANY(nx_epsilon_increment_t) nx_epsilon_increment;
} PSetByField;

#define FMT_ninf "f"
#define FMT_pinf "f"
#define FMT_nn "s"
#define FMT_wl_rank "ld"
#define FMT_wl_value "f"
#define FMT_windowing "s"
#define FMT_nx_epsilon_increment "f"

#define PSET_FMT(NAME) FMT_ ## NAME

#define FMT_PRE_ninf ninf_value
#define FMT_PRE_pinf pinf_value
#define FMT_PRE_nn NN_NAMES[nn_value]
#define FMT_PRE_wl_rank wl_rank_value
#define FMT_PRE_wl_value wl_value_value
#define FMT_PRE_windowing WINDOWING_NAMES[windowing_value]
#define FMT_PRE_nx_epsilon_increment nx_epsilon_increment_value

#define FMT_PRE_ninf_2(V) V
#define FMT_PRE_pinf_2(V) V
#define FMT_PRE_nn_2(V) NN_NAMES[V]
#define FMT_PRE_wl_rank_2(V) V
#define FMT_PRE_wl_value_2(V) V
#define FMT_PRE_windowing_2(V) WINDOWING_NAMES[V]
#define FMT_PRE_nx_epsilon_increment_2(V) V

#define PSET_FMT_PRE(NAME) FMT_PRE_ ## NAME
#define PSET_FMT_PRE_2(NAME, V) FMT_PRE_ ## NAME ## _2(V)

#define PSETPRINT(NAME) { NAME ## _t NAME ## _value = pset->NAME; printf("%s:\t%" PSET_FMT(NAME)"\n", #NAME, PSET_FMT_PRE(NAME)); }

#define multi_psetitem(FUN)\
FUN(ninf);\
FUN(pinf);\
FUN(nn);\
FUN(wl_rank);\
FUN(wl_value);\
FUN(windowing);\
FUN(nx_epsilon_increment);

void parameters_hash(PSet*);

MANY(PSet) parameters_fields2psets(TCPC(PSetByField) psetbyfield);
PSetByField parameters_psets2byfields(MANY(PSet)* psets);
void parameters_fields_free(PSetByField*);

#endif