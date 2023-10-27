#include "sources.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


void sourcelist_insert(SourcesLists lists, Source* source) {
    SourcesListItem** cursor = &lists[source->dgaclass].root;

    while (*cursor) {
        cursor = &(*cursor)->next;
    }

    (*cursor) = calloc(1, sizeof(SourcesListItem));

    (*cursor)->source = source;
    (*cursor)->next = NULL;
    lists[source->dgaclass].size++;
}

void sourceslists_toarray(SourcesLists lists, SourcesArrays arrays) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        arrays[cl].number = lists[cl].size;
        arrays[cl]._ = calloc(arrays[cl].number, sizeof(Source*));

        SourcesListItem* cursor = lists[cl].root;
        int32_t i = 0;
        while (cursor) {
            arrays[cl]._[i++] = cursor->source;
            cursor = cursor->next;
        }
    }
}