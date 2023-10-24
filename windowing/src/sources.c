#include "sources.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


void sourcelist_insert(SourcesLists* lists, Source* source) {
    {
        SourcesListItem** cursor = &lists->binary.root;

        while (*cursor) {
            cursor = &(*cursor)->next;
        }

        (*cursor) = calloc(1, sizeof(SourcesListItem));

        (*cursor)->source = source;
        (*cursor)->next = NULL;
        lists->binary.size++;
    }


    {
        SourcesListItem** cursor = &lists->multi[source->class_].root;

        while (*cursor) {
            cursor = &(*cursor)->next;
        }

        (*cursor) = calloc(1, sizeof(SourcesListItem));

        (*cursor)->source = source;
        (*cursor)->next = NULL;
        lists->multi[source->class_].size++;
    }
}

void sourceslists_toarray(SourcesLists* lists, SourcesArrays* arrays) {
    arrays->binary.number = lists->binary.size;
    arrays->binary._ = calloc(lists->binary.size, sizeof(Source*));

    {
        SourcesListItem* cursor = lists->binary.root;
        int32_t i = 0;
        while (cursor) {
            arrays->binary._[i++] = cursor->source;
            cursor = cursor->next;
        }
    }


    for (int32_t cl = 0; cl < N_CLASSES; cl++) {
        arrays->multi[cl].number = lists->multi[cl].size;
        arrays->multi[cl]._ = calloc(arrays->multi[cl].number, sizeof(Source*));

        SourcesListItem* cursor = lists->multi[cl].root;
        int32_t i = 0;
        while (cursor) {
            arrays->multi[cl]._[i++] = cursor->source;
            cursor = cursor->next;
        }
    }
}