#ifndef __WINDOWINGTESTBED2_H__
#define __WINDOWINGTESTBED2_H__

#include "common.h"

#include "parameters.h"
#include "wapply.h"
#include "windowing.h"

MAKEMANY(MANY(RWindowing));

MAKETETRA(MANY(MANY(RWindowing)));

typedef struct TestBed2 {
    MANY(RSource) sources;

    MANY(WSize) wsizes;

    MANY(MANY(RWindowing)) windowingss;
    
//  | sources | wsize
    MANY(Dataset0) datasets;
// |wsize|
} TestBed2;

TestBed2* testbed2_create(MANY(WSize) wsizes);

void testbed2_source_add(TestBed2* tb2, __Source* source);

void testbed2_apply(TestBed2* tb2, const MANY(PSet) pset);
void testbed2_apply_free(TestBed2* tb2, const MANY(PSet) psets);

void testbed2_windowing(TestBed2*);

void testbed2_free(TestBed2*);

#endif