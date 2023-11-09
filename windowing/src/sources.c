#include "sources.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int32_t _index(DGA(List) lists) {
    int32_t index = 0;
    DGAFOR(cl) {
        index += lists[cl].size;
    }
    return index;
}

int32_t _binary_index(DGA(List) lists, Source* source) {
    int32_t bindex = 0;
    if (source->dgaclass == DGACLASS_0) {
        bindex = lists[DGACLASS_0].size;
    } else {
        bindex = lists[DGACLASS_1].size + lists[DGACLASS_2].size;
    }
    return bindex;
}

void sources_list_insert(DGA(List) lists, Source* source) {
    list_insert(&lists[source->dgaclass], source);
}

void sources_lists_to_arrays(DGA(List) lists, DGA(Many) arrays) {
    DGAFOR(cl) {
        arrays[cl] = list_to_array(lists[cl]);
    }
}

void sources_lists_free(DGA(List) lists) {
    DGAFOR(cl) {
        list_free(lists[cl], 1);
    }
}

void sources_arrays_free(DGA(Many) arrays) {
    DGAFOR(cl) {
        array_free(arrays[cl]);
    }
}