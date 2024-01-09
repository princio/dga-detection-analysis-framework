
#include "tb2w.h"

#include "dataset.h"
#include "gatherer.h"
#include "io.h"
#include "logger.h"
#include "stratosphere.h"
#include "tb2w_io.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

size_t _tb2w_get_index(MANY(RSource) sources) {
    size_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
}

RTB2W tb2w_create(char rootdir[DIR_MAX], size_t wsize, const ParameterGenerator pg) {
    RTB2W tb2w = calloc(1, sizeof(TB2W));
    memset(tb2w, 0, sizeof(TB2W));

    strcpy(tb2w->rootdir, rootdir);

    configsuite_generate(&tb2w->configsuite, pg);

    tb2w->wsize = wsize;

    tb2w->sources.element_size = sizeof(RSource);
    
    return tb2w;
}

void tb2w_source_add(RTB2W tb2, RSource source) {
    GATHERER_ADD(tb2->sources, 5, source, RSource);

    SourceIndex walker;
    memset(&walker, 0, sizeof(SourceIndex));
    for (size_t i = 0; i < tb2->sources.number - 1; i++) {
        walker.all++;
        walker.binary += tb2->sources._[i]->wclass.bc == source->wclass.bc;
        walker.multi += tb2->sources._[i]->wclass.mc == source->wclass.mc;
    }

    source->index = walker;
}

void tb2w_windowing(RTB2W tb2) {
    gatherer_many_finalize((__MANY*) &tb2->sources, sizeof(RSource));

    BY_SETN(tb2->windowing, source, tb2->sources.number);
    MANY_INIT(tb2->windowing.bysource, tb2->sources.number, RWindow);

    int32_t count_windows[N_DGACLASSES];
    memset(&count_windows, 0, sizeof(int32_t) * N_DGACLASSES);
    BY_FOR(tb2->windowing, source) {
        RSource source;
        RWindowing* windowing_ref;

        source = tb2->sources._[idxsource];
        windowing_ref = &BY_GET(tb2->windowing, source);

        *windowing_ref = windowings_create(tb2->wsize, source);
        count_windows[source->wclass.mc] += (*windowing_ref)->windows.number;
    }

    BY_FOR(tb2->windowing, source) {
        RWindowing windowing = BY_GET(tb2->windowing, source);
        for (size_t w = 0; w < windowing->windows.number; w++) {
            MANY_INIT(windowing->windows._[w]->applies, tb2->configsuite.configs.number, WApply);
        }
    }
}

void tb2w_apply(RTB2W tb2w) {
    #define DELLINE "\033[A\033[2K"
    BY_FOR(tb2w->windowing, source) {
        printf(DELLINE"%d %ld/%ld -> doing\n", tb2w->windowing.bysource._[idxsource]->source->id, idxsource, tb2w->windowing.n.source);
        stratosphere_apply(tb2w, BY_GET(tb2w->windowing, source));
    }
    #undef DELLINE
}

void tb2w_free(RTB2W tb2w) {
    FREEMANY(tb2w->windowing.bysource);
    FREEMANY(tb2w->sources);
    free(tb2w);
}
