
#ifndef __SOURCES_H__
#define __SOURCES_H__

#include "common.h"

#include "list.h"

typedef struct SourceIndex {
    int32_t galaxy;
    int32_t source_galaxy;
    
    int32_t all;
    int32_t binary;
    int32_t multi;
} SourceIndex;

typedef struct Source {
    SourceIndex index;

    int32_t parent_galaxy_index;

    char name[50];

    DGAClass dgaclass;
    CaptureType capture_type;

    int32_t id;
    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t fnreq_max;
} Source;

MAKEMANY(Source);
MAKEDGA(Source);
MAKEDGAMANY(Source);
MAKEMANYDGA(Source);

typedef Source* RSource;
MAKEMANY(RSource);
MAKEDGAMANY(RSource);

typedef struct Galaxy {
    int32_t index;
    char name[50];
    DGAMANY(RSource) sources;
} Galaxy;

MAKEMANY(Galaxy);

MAKEDGA(List);
MAKEDGA(Many);

void sources_list_findbyid(DGA(List) lists, Source* source);

void sources_list_insert(DGA(List) lists, Source* source);

void sources_lists_to_arrays(DGA(List) lists, DGA(Many) arrays);

void sources_lists_free(DGA(List) lists);

void sources_arrays_free(DGA(Many) arrays);

#endif