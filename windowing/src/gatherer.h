
#ifndef __GATHERER_H__
#define __GATHERER_H__

#include "common.h"

typedef void (*GathererFreeFn)(void *);

typedef struct __Gatherer* RGatherer;
typedef struct __Gatherer {
    char name[60];

    RGatherer* ref;

    size_t size_realloc_increment;
    size_t element_size;

    GathererFreeFn freefn;
    
    __MANY many;
} __Gatherer;

#define GATHERER_ADD(MANY, SRI, ELEMENT, T)\
    gatherer_many_add((__MANY*) &(MANY), (SRI), (uint8_t*) (&ELEMENT), sizeof(T));


void gatherer_many_add(__MANY* many, size_t size_realloc_increment, uint8_t* element, size_t element_size);
void gatherer_many_realloc(__MANY* many, size_t element_size, size_t size_realloc_increment);
void gatherer_many_finalize(__MANY* many, size_t element_size);
void gatherer_alloc(RGatherer*, char*, GathererFreeFn, size_t initial_size, size_t element_size, size_t size_realloc_increment);
void* gatherer_alloc_item(RGatherer gat);
void gatherer_free_all();

#endif