#include "sources.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int32_t _index(DGA(List) lists) {
    int32_t index = 0;
    EXEC_ARRAY_ALLDGA(index += lists[DGACURSOR].size);
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

    source->index = _index(lists);
    source->binary_index = _binary_index(lists, source);
    source->multi_index = lists[source->dgaclass].size;
}

void sources_lists_to_arrays(DGA(List) lists, DGA(Many) arrays) {
    EXEC_ARRAY_ALLDGA(arrays[DGACURSOR] = list_to_array(lists[DGACURSOR]));
}

void sources_lists_free(DGA(List) lists) {
    EXEC_ARRAY_ALLDGA(list_free(lists[DGACURSOR], 1));
}

void sources_arrays_free(DGA(Many) arrays) {
    EXEC_ARRAY_ALLDGA(array_free(arrays[DGACURSOR]));
}