#include "source.h"

#include "gatherer2.h"
// #include "logger.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void _source_free(void*);
void _source_io(IOReadWrite, FILE*, void**);
void _source_print(void*);
void _source_hash(void*, SHA256_CTX*);

G2Config g2_config_source = {
    .element_size = sizeof(__Source),
    .size = 0,
    .freefn = _source_free,
    .iofn = _source_io,
    .printfn = _source_print,
    .hashfn = _source_hash,
    .id = G2_SOURCE
};

RSource source_alloc() {
    return (RSource) g2_insert_alloc_item(G2_SOURCE);
}

void _source_free(void* item) {
}

void _source_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    RSource* source = (RSource*) item;

    size_t sources_number = 0;

    FRWSIZE((*source)->index, sizeof(__Source) - sizeof(G2Index));
}

void _source_print(void* item) {
    RSource source = (RSource) item;

    printf("%10s: %ld\n", "g2index", source->g2index);
    printf("%10s: %s\n", "name", source->name);
    printf("%10s: %s\n", "galaxy", source->galaxy);
    printf("%10s: %d\n", "wclass.bc", source->wclass.bc);
    printf("%10s: %d\n", "wclass.mc", source->wclass.mc);
    printf("%10s: %d\n", "id", source->id);
    printf("%10s: %ld\n", "qr", source->qr);
    printf("%10s: %ld\n", "q", source->q);
    printf("%10s: %ld\n", "r", source->r);
    printf("%10s: %ld\n", "fnreq_max", source->fnreq_max);
    printf("%10s: %d\n", "day", source->day);
}

#define MISM(A) { int _m = (A); if ((A)) LOG_ERROR(#A); mismatch += _m; }
int source_cmp(RSource a, RSource b) {
    int mismatch = 0;

    MISM(memcmp(a, b, sizeof(__Source)));

    return mismatch;
}

void _source_hash(void* item, SHA256_CTX* sha) {
    RSource source = (RSource) item;

    SHA256_Update(sha, source, sizeof(__Source));
}