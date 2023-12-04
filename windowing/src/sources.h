
#ifndef __SOURCES_H__
#define __SOURCES_H__

#include "common.h"

#include "parameters.h"

#include "io.h"
#include "list.h"

#define MAX_SOURCEs 100

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

typedef struct __Source {
    int32_t index;

    char name[50];
    char galaxy[50];

    WClass wclass;
    
    CaptureType capture_type;
    SourceWindowingExecution windowing_type;

    int32_t id;
    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t fnreq_max;
} __Source;

MAKEMANY(RSource);

MAKETETRA(MANY(RSource));

int32_t sources_add(MANY(RSource)* sources, RSource source);
RSource sources_alloc();
void sources_free();
void sources_finalize(MANY(RSource)* sources);

void sources_io(IOReadWrite, FILE*, void*);
void sources_io_objid(TCPC(void), char[IO_OBJECTID_LENGTH]);



#endif