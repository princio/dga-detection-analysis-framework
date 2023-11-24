
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include "common.h"

#include "io.h"

typedef struct PSet {
    int32_t id;
    
    InfiniteValues infinite_values;
    NN nn;
    Whitelisting whitelisting;
    WindowingType windowing;
} PSet;

typedef struct PSetGenerator {
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

void parameters_hash(PSet*);

void parameters_print(PSet* pset);

MANY(PSet) parameters_generate(TCPC(PSetGenerator));

void parameters_io(IOReadWrite, FILE*, void*);
void parameters_io_objid(TCPC(void), char[IO_OBJECTID_LENGTH]);

#endif