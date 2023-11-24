#include "sources.h"

#include "cache.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

MAKEMANY(__Source);

MANY(RSource) sources_gatherer = {
    .number = 0,
    ._ = NULL
};

void _sources_realloc(MANY(RSource)* sources, int32_t index) {
    assert(index <= sources->number);

    if (sources->number == index) {
        const int new_number = sources->number + 50;
    
        sources->_ = realloc(sources->_, (new_number) * sizeof(RSource));
        sources->number = new_number;

        for (int32_t s = index; s < new_number; s++) {
            sources->_[s] = NULL;
        }
    }
}

int32_t _sources_index(TCPC(MANY(RSource)) sources) {
    int32_t s;

    for (s = 0; s < sources->number; s++) {
        if (sources->_[s] == NULL) break;
    }

    return s;
}

int32_t sources_add(MANY(RSource)* sources, RSource source) {
    const int32_t index = _sources_index(sources);
    _sources_realloc(sources, index);
    sources->_[index] = source;
    return index;
}

void sources_tetra_add(TETRA(MANY(RSource))* sources, RSource source) {
    sources_add(&sources->all, source);
    sources_add(&sources->binary[source->wclass.bc], source);
    sources_add(&sources->multi[source->wclass.mc], source);
}

RSource sources_alloc() {
    RSource source = calloc(1, sizeof(__Source));

    const int32_t index = sources_add(&sources_gatherer, source);

    source->index = index;
    
    sources_gatherer._[index] = source;

    return source;
}

void sources_finalize(MANY(RSource)* sources) {
    const int32_t index = _sources_index(sources);
    sources->number = index + 1;
    sources->_ = realloc(sources->_, (sources->number) * sizeof(RSource));
}

void sources_free() {
    for (int32_t s = 0; s < sources_gatherer.number; s++) {
        if (sources_gatherer._[s] == NULL) break;
        free(sources_gatherer._[s]);
    }
    free(sources_gatherer._);
}

void sources_io(IOReadWrite rw, FILE* file, void*obj) {
    __Source *source = obj;

    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(source->name);
    FRW(source->galaxy);

    FRW(source->wclass);

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

    io_subdigest(obj, sizeof(__Source), subdigest);
    sprintf(objid, "sources_%s", subdigest);
}