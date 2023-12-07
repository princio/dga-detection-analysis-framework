#ifndef __WINDOWINGTESTBED2_H__
#define __WINDOWINGTESTBED2_H__

#include "common.h"

#include "dataset.h"
#include "fold.h"
#include "parameters.h"
#include "wapply.h"
#include "windowing.h"

#define TB2_WSIZES_N(A) A->wsizes.number

MAKEMANY(MANY(RWindowing));

MAKETETRA(MANY(MANY(RWindowing)));

typedef struct TB2WindowingsSource {
    MANY(RWindowing) bywsize;
} TB2WindowingsSource;
MAKEMANY(TB2WindowingsSource);

typedef struct TestBed2Windowings {
    MANY(TB2WindowingsSource) bysource;
} TestBed2Windowings;

typedef struct TestBed2Datasets {
    MANY(RDataset0) bywsize;
} TestBed2Datasets;

typedef struct __TestBed2 {
    int32_t applied;
    MANY(WSize) wsizes;
    MANY(PSet) psets;

    MANY(RSource) sources;
    TestBed2Windowings windowings;
    TestBed2Datasets datasets;

    MANY(RFold) folds;
} __TestBed2;

RTestBed2 testbed2_create(MANY(WSize) wsizes);
void testbed2_source_add(RTestBed2 tb2, RSource source);
void testbed2_apply(RTestBed2 tb2, MANY(PSet) pset);
void testbed2_windowing(RTestBed2);
void testbed2_fold_add(RTestBed2, FoldConfig);
void testbed2_free(RTestBed2);

void testbed2_io_dataset(IOReadWrite rw, FILE* file, RTestBed2 tb2, const WSize wsize, RDataset0* ds_ref);
void testbed2_io(IOReadWrite rw, char dirname[200], RTestBed2* tb2, int applied);

#endif