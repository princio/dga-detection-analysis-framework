
#ifndef __WINDOWING_SOURCE_H__
#define __WINDOWING_SOURCE_H__

#include "common.h"

#include "gatherer2.h"
#include "io.h"
#include "list.h"

typedef struct SourceIndex {
    size_t all;
    size_t binary;
    size_t multi;
} SourceIndex;

typedef struct __Source {
    G2Index g2index;

    SourceIndex index;

    char name[100];
    char galaxy[50];

    WClass wclass;
    
    int32_t id;
    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t fnreq_max;
    
    int day;
    int days;
} __Source;

MAKEMANY(__Source);
MAKEMANY(RSource);

RSource source_alloc();

#endif