
#ifndef __TETRA_H__
#define __TETRA_H__

#include "common.h"

typedef struct TetraX {
    size_t all;
    size_t binary[2];
    size_t multi[N_DGACLASSES];
} TetraX;

MAKEMANY(__MANY);
MAKEMANY(MANY(__MANY));

typedef struct Tetra {
    __MANY all;
    __MANY binary[2];
    __MANY multi[N_DGACLASSES];
} Tetra;

typedef struct TetraMany {
    MANY(__MANY) all;
    MANY(__MANY) binary[2];
    MANY(__MANY) multi[N_DGACLASSES];
} TetraMany;

typedef struct TetraItem {
    __MANY* all;
    __MANY* binary;
    __MANY* multi;
} TetraItem;

void tetra_init(Tetra* tetra, const TetraX tx, const size_t element_size);
void tetra_init_with_tetra(Tetra* tetra, Tetra const * const tetra2, const size_t element_size);
TetraItem tetra_get(Tetra* tetra, const TetraX tx, const WClass wc);
void tetra_apply_all(Tetra* tetra, void fn(void*));
void tetra_apply(Tetra* tetra, TetraX const tx, const WClass wc, void *fn(void*));
void tetra_set_many(TetraMany* tetra, TetraX* tx, const WClass wc, const int32_t index_many, void* item);
void tetra_increment(TetraX* tx, const WClass wc, const int32_t inc);
void tetra_free(Tetra* tetra);

#endif