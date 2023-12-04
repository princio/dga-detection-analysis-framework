#ifndef __WINDOWINGTESTBED2_H__
#define __WINDOWINGTESTBED2_H__

#include "common.h"

#include "parameters.h"
#include "wapply.h"
#include "windowing.h"

MAKEMANY(MANY(RWindowing));

MAKETETRA(MANY(MANY(RWindowing)));

typedef struct TB2WindowingsSource {
    MANY(RWindowing) wsize;
} TB2WindowingsSource;
MAKEMANY(TB2WindowingsSource);

typedef struct TestBed2Windowings {
    MANY(TB2WindowingsSource) source;
} TestBed2Windowings;

typedef struct TestBed2Datasets {
    MANY(RDataset0) wsize;
} TestBed2Datasets;

typedef struct TestBed2 {
    MANY(RSource) sources;

    MANY(WSize) wsizes;

    TestBed2Windowings windowings;

    TestBed2Datasets datasets;
// |wsize|

    int32_t applied;
    MANY(PSet) psets;
} TestBed2;

TestBed2* testbed2_create(MANY(WSize) wsizes);

void testbed2_source_add(TestBed2* tb2, __Source* source);

void testbed2_apply(TestBed2* tb2, const MANY(PSet) pset);
void testbed2_apply_free(TestBed2* tb2, const MANY(PSet) psets);

void testbed2_windowing(TestBed2*);

void testbed2_free(TestBed2*);

#endif