#include "source.h"

#include "gatherer2.h"
// #include "logger.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void _source_free(void* item);
void _source_io(IOReadWrite rw, FILE* file, void**);

G2Config g2_config_source = {
    .element_size = sizeof(__Source),
    .size = 0,
    .freefn = _source_free,
    .iofn = _source_io,
    .id = G2_SOURCE
};

RSource source_alloc() {
    return (RSource) g2_insert_and_alloc(G2_SOURCE);
}

void source_io(IOReadWrite rw, FILE* file, void* item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    RSource source = (RSource) item;

    size_t sources_number = 0;

    tb2_io_flag_source(rw, file, 0, __LINE__);
    
    FRW(source->index);
    FRW(source->galaxy);
    FRW(source->name);
    FRW(source->wclass);
    FRW(source->id);
    FRW(source->qr);
    FRW(source->q);
    FRW(source->r);
    FRW(source->fnreq_max);

    tb2_io_flag_source(rw, file, source->id, __LINE__);
}