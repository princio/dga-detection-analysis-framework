#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "parameters.h"
#include "sources.h"
#include "windows.h"

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

typedef struct Windowing {
    RSource source;
    
    PSet* pset;

    MANY(Window) windows;
} Windowing;

typedef Windowing* RWindowing;

MAKEMANY(Windowing);
MAKEMANY(RWindowing);
MAKEDGA(Windowing);
MAKEDGA(RWindowing);

MAKEDGAMANY(Windowing);
MAKEDGAMANY(RWindowing);

typedef void (*WindowingFunction)(TCPC(Source), MANY(RWindowing) const);

typedef struct WindowingOrigin {
    MANY(Galaxy) galaxies;

    struct {
        MANY(Source) all;
        MANY(RSource) binary[2];
        DGAMANY(RSource) multi;
    } sources;
} WindowingOrigin;

typedef void (*WindowingAPFunction)(TCPC(Source), MANY(PSet), int32_t[], MANY(Windowing));

void windowing_domainname(const DNSMessage message, TCPC(Windowing) windowing);

MANY(Windowing) windowing_run_1source_manypsets(TCPC(Source) source, MANY(PSet) psets, WindowingAPFunction fn);

#endif