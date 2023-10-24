
#ifndef __SOURCES_H__
#define __SOURCES_H__

#include "common.h"

typedef struct Source {
    int32_t binary_index;
    int32_t multi_index;
    
    int32_t galaxy_id;

    CaptureType capture_type;

    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t nmessages;
    int64_t fnreq_max;

    char name[50];
    char source[50];

    Class class_;
} Source;

typedef Source* SourcePtr;


typedef struct Sources {
    int32_t number;
    Source* _;
} Sources;


typedef struct SourcesRef {
    int32_t number;
    Source** _;
} SourcesRef;

typedef struct SourcesListItem {
    Source* source;
    struct SourcesListItem* next; 
} SourcesListItem;

typedef struct SourcesList {
    int32_t size;
    struct SourcesListItem* root;
} SourcesList;

typedef struct SourcesArray {
    int32_t number;
    Source** _;
} SourcesArray;

typedef struct SourcesLists {
    SourcesList binary;
    SourcesList multi[N_CLASSES];
} SourcesLists;

typedef struct SourcesArrays {
    SourcesArray binary;
    SourcesArray multi[N_CLASSES];
} SourcesArrays;



void sourcelist_insert(SourcesLists* lists, Source* source);

void sourceslists_toarray(SourcesLists* lists, SourcesArrays* arrays);


#endif