#include "gatherer.h"


#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MANY_GET(MANY, N) ((uint8_t*) (MANY)._)[N * (MANY).element_size]

MAKEMANY(__Gatherer);

MANY(__Gatherer) gatherer_of_gatherers = {
    .size = 0,
    .number = 0,
    ._ = NULL
};

void gatherer_many_add(__MANY* many, size_t element_size, size_t size_realloc_increment, void* element) {
    gatherer_many_realloc(many, element_size, size_realloc_increment);
    memcpy(&many->_[many->number * element_size], &element, element_size);
    many->number++;
}

void gatherer_many_realloc(__MANY* many, size_t element_size, size_t size_realloc_increment) {
    if (many->size == 0) {
        many->size += size_realloc_increment;
        many->_ = calloc(many->size, element_size);
    } else if (many->size == many->number) {
        many->size += size_realloc_increment;
        many->_ = realloc(many->_, many->size * element_size);
    }
}

void gatherer_many_finalize(__MANY* many, size_t element_size) {
    if (many->size > many->number) {
        many->_ = realloc(many->_, many->number * element_size);
    }
}

void gatherer_alloc(RGatherer* gatherer_ref, char name[50], GathererFreeFn freefn, size_t initial_size, size_t element_size, size_t size_realloc_increment) {
    RGatherer gatherer;
    {
        if (gatherer_of_gatherers.size == 0) {
            INITMANY(gatherer_of_gatherers, 10, __Gatherer);
            gatherer_of_gatherers.number = 0;
        }
        gatherer_many_realloc((__MANY*) &gatherer_of_gatherers, sizeof(__Gatherer), 5);
    }
    gatherer = &gatherer_of_gatherers._[gatherer_of_gatherers.number];
    gatherer_of_gatherers.number++;
    
    strcpy(gatherer->name, name);

    gatherer->ref = gatherer_ref;
    gatherer->freefn = freefn;
    gatherer->element_size = element_size;
    gatherer->size_realloc_increment = size_realloc_increment;

    INITMANYSIZE(gatherer->many, initial_size, element_size);
    gatherer->many.number = 0;
    
    *gatherer_ref = gatherer;
}

void* gatherer_alloc_item(RGatherer gat) {
    void* allocated = NULL;

    gatherer_many_realloc((__MANY*) &gat->many, gat->element_size, gat->size_realloc_increment);

    allocated = (void*) &MANY_GET(gat->many, gat->many.number);

    gat->many.number++;

    return allocated;
}

void gatherer_free_all() {
    for (size_t s = 0; s < gatherer_of_gatherers.number; s++) {
        for (size_t i = 0; i < gatherer_of_gatherers._[s].many.number; i++) {
            if (gatherer_of_gatherers._[s].freefn) {
                gatherer_of_gatherers._[s].freefn((void*) &MANY_GET(gatherer_of_gatherers._[s].many, i));
            }
        }
        *gatherer_of_gatherers._[s].ref = NULL;
        FREEMANY(gatherer_of_gatherers._[s].many);
    }
    FREEMANY(gatherer_of_gatherers);
}