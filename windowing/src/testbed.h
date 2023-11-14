#ifndef __WINDOWINGTESTBED_H__
#define __WINDOWINGTESTBED_H__

#include "dataset.h"
#include "parameters.h"
#include "windowing.h"

typedef struct TestBedWindowing {
    MANY(Windowing) all;
    MANY(Windowing) binary[2];
    DGAMANY(Windowing) multi;
} TestBedWindowing;

typedef struct TestBedWindows {
    MANY(RWindow) all;
    MANY(RWindow) binary[2];
    DGAMANY(RWindow) multi;
} TestBedWindows;

typedef struct TestBedPSetApply {
    TestBedWindowing windowings; // composed by N_SOURCEs elements
    TestBedWindows windows; // composed by one element.
} TestBedPSetApply;

typedef struct TestBedSources {
    MANY(Source) all;
    MANY(RSource) binary[2];
    DGAMANY(RSource) multi;
} TestBedSources;

/// @brief This is needed because of performance reasons.
/// We cannot perform Windowing one source and one parameter set per time, it would require too much time.
/// For this reason, we perform $p$ parameter sets for noe source per time.
typedef struct TestBed {
    MANY(PSet) psets;

    MANY(Galaxy) galaxies; // UNUSED

    TestBedSources sources;

    TestBedPSetApply* applies; // composed by N_PSETs elements
} TestBed;

void testbed_source_add(Source* source, int type, WindowingAPFunction fn);

void testbed_init(TCPC(PSetGenerator) psetgen);

TestBed testbed_run();

void testbed_free(TestBed* tb);

#endif