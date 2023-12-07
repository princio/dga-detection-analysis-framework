
#ifndef __GATHERER_H__
#define __GATHERER_H__

#include "common.h"

typedef void (*GathererFreeFn)(void *);

typedef struct __Gatherer {
    char name[50];

    size_t size_realloc_increment;
    size_t element_size;

    GathererFreeFn freefn;
    
    __MANY many;
} __Gatherer;

typedef __Gatherer* RGatherer;

void gatherer_many_add(__MANY* many, size_t element_size, size_t size_realloc_increment, void* element);
void gatherer_many_realloc(__MANY* many, size_t element_size, size_t size_realloc_increment);
void gatherer_many_finalize(__MANY* many, size_t element_size);
RGatherer gatherer_create(char name[50], GathererFreeFn, size_t initial_size, size_t element_size, size_t size_realloc_increment);
void* gatherer_alloc(RGatherer gat);
void gatherer_free_all();

#endif