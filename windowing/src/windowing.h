#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "parameters.h"
#include "sources.h"
#include "windows0.h"

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

// typedef void (*ApplySourceFunction)(struct Source*, MANY(ApplyPSet) ap, struct MANY(RWindow) windows_pset[ap.number]);
// typedef void (*ApplyWindowFunction)(TCPC(struct Source), struct MANY(PSet) psets, const int32_t fnreq_min, const int32_t fnreq_max, const int32_t wnum, struct Window* windows[psets.number]);

/*
typedef struct ApplyWindow {
    TCPC(Source) source;
    MANY(PSet) psets;
    int32_t wnum;
    uint64_t fnreq_min;
    uint64_t fnreq_max;

    ApplyWindowFunction fn;
} ApplyWindow;

typedef struct ApplyPSet {
    WSize wsize;
    PSet* pset;
    int loaded;
} ApplyPSet;

MAKEMANY(ApplyPSet);

typedef struct ApplySource {
    TCPC(Source) source;

    MANY(ApplyPSet) applies;

    ApplySourceFunction fn;
} ApplySource;

typedef struct SourceApply {
    Source* source;
    PSet* pset;
    WSize wsize;
    
    MANY(RWindow) windows;
} SourceApply;

MAKEMANY(SourceApply);
MAKEDGAMANY(SourceApply);
*/

typedef struct __Windowing {
    int32_t index;
    __Source* source;
    WSize wsize;
    MANY(RWindow0) windows;
} __Windowing;

// typedef __Windowing* RWindowing; in common.h

MAKEMANY(RWindowing);

MAKETETRA(MANY(RWindowing));

void windowings_add(MANY(RWindowing)* windowings, RWindowing windowing);

RWindowing windowings_create(RSource rsource, WSize wsize);

void windowings_finalize(MANY(RWindowing)* windowings);
void windowings_free();

// int windowing_load(T_PC(SourceApply) sourceapply);
// int windowing_save(TCPC(SourceApply));

// MANY(SourceApply) windowing_run_source(TCPC(ApplySource) as);

// void windowing_io(IOReadWrite, FILE*, void*);
// void windowing_io_objid(TCPC(void), char[IO_OBJECTID_LENGTH]);

#endif