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
int sources_gatherer_initialized = 0;

void _sources_realloc(MANY(RSource)* sources, size_t index) {
    assert(index <= sources->number);

    if (sources->number <= index) {
        const size_t new_number = sources->number + 50;
    
        sources->_ = realloc(sources->_, (new_number) * sizeof(RSource));
        sources->number = new_number;

        for (size_t s = index; s < new_number; s++) {
            sources->_[s] = NULL;
        }
    }
}

size_t _sources_emptyslot_index(TCPC(MANY(RSource)) sources) {
    size_t s;

    for (s = 0; s < sources->number; s++) {
        if (sources->_[s] == NULL) break;
    }

    return s;
}

size_t sources_add(MANY(RSource)* sources, RSource source) {
    const size_t index = _sources_emptyslot_index(sources);
    _sources_realloc(sources, index);
    sources->_[index] = source;
    
    memset(&source->index, 0, sizeof(Index));

    source->index.all = index;

    for (size_t i = 0; i < index; i++) {
        if (sources->_[i]->wclass.bc == source->wclass.bc) {
            source->index.binary++;
        }
        if (sources->_[i]->wclass.mc == source->wclass.mc) {
            source->index.multi++;
        }
    }

    return index;
}

RSource sources_alloc() {
    RSource source = calloc(1, sizeof(__Source));

    if (sources_gatherer_initialized == 0) {
        INITMANY(sources_gatherer, 50, RSource);
        sources_gatherer_initialized = 1;
    }

    sources_add(&sources_gatherer, source);

    return source;
}

void sources_finalize(MANY(RSource)* sources) {
    const int32_t index = _sources_emptyslot_index(sources);
    sources->number = index;
    sources->_ = realloc(sources->_, (sources->number) * sizeof(RSource));
}

void sources_free() {
    for (size_t s = 0; s < sources_gatherer.number; s++) {
        if (sources_gatherer._[s] == NULL) break;
        free(sources_gatherer._[s]);
    }
    FREEMANY(sources_gatherer);
    sources_gatherer_initialized = 0;
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