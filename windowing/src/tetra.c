#include "tetra.h"

#include <stdlib.h>

void tetra_init(Tetra* tetra, const TetraX tx, const size_t element_size) {
    INITMANYSIZE(tetra->all, tx.all, element_size);
    INITMANYSIZE(tetra->binary[0], tx.binary[0], element_size);
    INITMANYSIZE(tetra->binary[1], tx.binary[1], element_size);
    DGAFOR(cl) INITMANYSIZE(tetra->multi[cl], tx.multi[cl], element_size);
}

void tetra_init_with_tetra(Tetra* tetra, Tetra const * const tetra2, const size_t element_size) {
    TetraX tx;

    tx.all = tetra2->all.number;

    tx.binary[0] = tetra2->binary[0].number;
    tx.binary[1] = tetra2->binary[1].number;

    tx.multi[0] = tetra2->multi[0].number;
    tx.multi[1] = tetra2->multi[1].number;
    tx.multi[2] = tetra2->multi[2].number;

    tetra_init(tetra, tx, element_size);
}

TetraItem tetra_get(Tetra* tetra, const TetraX tx, const WClass wc) {
    TetraItem ti;
    ti.all = tetra->all._[tx.all];
    ti.binary = tetra->binary[wc.bc]._[tx.binary[wc.bc]];
    ti.multi = tetra->multi[wc.mc]._[tx.multi[wc.mc]];
    return ti;
}

void tetra_apply_all(Tetra* tetra, void fn(void*)) {
    fn(&tetra->all);
    fn(&tetra->binary[0]);
    fn(&tetra->binary[1]);
    DGAFOR(cl) fn(&tetra->multi[cl]);
}

void tetra_apply(Tetra* tetra, TetraX const tx, const WClass wc, void *fn(void*)) {
    fn(&tetra->all._[tx.all]);
    fn(&tetra->binary[wc.bc]._[tx.binary[wc.bc]]);
    fn(&tetra->multi[wc.mc]._[tx.multi[wc.mc]]);
}

void tetra_apply_many(TetraMany* tetra, TetraX const tx, const WClass wc, void *fn(void*)) {
    fn(&tetra->all._[tx.all]._);
    fn(&tetra->binary[wc.bc]._[tx.binary[wc.bc]]);
    fn(&tetra->multi[wc.mc]._[tx.multi[wc.mc]]);
}

void tetra_set(Tetra* tetra, TetraX* tx, const WClass wc, void* item) {
    tetra->all._[tx->all] = item;
    tetra->binary[wc.bc]._[tx->binary[wc.bc]] = item;
    tetra->multi[wc.mc]._[tx->multi[wc.mc]] = item;
}

void tetra_set_many(TetraMany* tetra, TetraX* tx, const WClass wc, const int32_t index_many, void* item) {
    tetra->all._[tx->all]._[index_many] = item;
    tetra->binary[wc.bc]._[tx->binary[wc.bc]]._[index_many] = item;
    tetra->multi[wc.mc]._[tx->multi[wc.mc]]._[index_many] = item;
}

void tetra_increment(TetraX* tx, const WClass wc, const int32_t inc) {
    tx->all += inc;
    tx->binary[wc.bc] += inc;
    tx->multi[wc.mc] += inc;
}

void tetra_free(Tetra* tetra) {
    tetra_apply_all(tetra, free);
}