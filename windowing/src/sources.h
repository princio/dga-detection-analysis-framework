
#ifndef __SOURCES_H__
#define __SOURCES_H__

#include "common.h"

#include "list.h"


typedef enum SourceWindowingExecution {
    SOURCE_WINDOWING_TYPE_SINGLE,
    SOURCE_WINDOWING_TYPE_MANYPSETs,
    SOURCE_WINDOWING_TYPE_MANYSOURCEs
} SourceWindowingExecution;

typedef struct SourceIndex {
    int32_t galaxy;
    int32_t source_galaxy;
    
    int32_t all;
    int32_t binary;
    int32_t multi;
} SourceIndex;

struct Galaxy;

typedef struct Source {
    // SourceIndex index;

    // int32_t parent_galaxy_index;

    char name[50];
    char galaxy[50];

    BinaryClass binaryclass;
    DGAClass dgaclass;
    
    CaptureType capture_type;
    SourceWindowingExecution windowing_type;

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

typedef TCP_(Source) RSource;
MAKEMANY(RSource);
MAKEDGAMANY(RSource);

typedef struct Galaxy {
    char name[50];

    struct {
        MANY(Source) all;
        MANY(RSource) binary[2];
        DGAMANY(RSource) multi;
    } sources;
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