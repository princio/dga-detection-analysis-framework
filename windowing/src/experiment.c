
#include "experiment.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

void experiment_sources_add(Experiment* exp, Source* source) {
    sourcelist_insert(&exp->sources_lists, source);
}

void experiment_sources_fill(Experiment* exp) {
    sourceslists_toarray(&exp->sources_lists, &exp->sources_arrays);
}