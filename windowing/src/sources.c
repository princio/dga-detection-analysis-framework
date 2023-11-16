#include "sources.h"

#include "cache.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void sources_io(IOReadWrite rw, FILE* file, void*obj) {
    Source *source = obj;

    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(source->name);
    FRW(source->galaxy);

    FRW(source->binaryclass);
    FRW(source->dgaclass);

    FRW(source->capture_type);
    FRW(source->windowing_type);

    FRW(source->id);
    FRW(source->qr);
    FRW(source->q);
    FRW(source->r);
    FRW(source->fnreq_max);
}

void sources_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    char subdigest[IO_DIGEST_LENGTH];

    memset(objid, 0, IO_OBJECTID_LENGTH);

    io_subdigest(obj, sizeof(Source), subdigest);
    sprintf(objid, "sources_%s", subdigest);
}