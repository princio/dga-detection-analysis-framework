
#include "testbed.h"

#include "common.h"
#include "parameters.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

struct ExecSource {
    Source* source;
    SourceWindowingExecution type;
    WindowingAPFunction fn;
};

struct Index {
    int32_t all;
    int32_t binary[2];
    int32_t multi[N_DGACLASSES];
} sources_count;

MANY(PSet) psets;
List galaxies;
List execsources_list;

void testbed_init(TCPC(PSetGenerator) psetgen) {
    memset(&sources_count, 0, sizeof(sources_count));
    psets = parameters_generate(psetgen);

    list_init(&execsources_list, sizeof(struct ExecSource));

    printf("%d\n", execsources_list.element_size);
}

void testbed_source_add(Source* source, int type, WindowingAPFunction fn) {
    struct ExecSource* exec_source = calloc(1, sizeof(struct ExecSource));
    exec_source->fn = fn;
    exec_source->source = source;
    exec_source->type = type;

    list_insert(&execsources_list, exec_source);

    ++sources_count.all;
    ++sources_count.binary[source->dgaclass > 0];
    ++sources_count.multi[source->dgaclass];
}

TestBed testbed_run() {
    TestBed testbed;
    struct ExecSource **execsources;

    memset(&testbed, 0, sizeof(TestBed));
    testbed.applies = calloc(psets.number, sizeof(TestBedPSetApply));
    testbed.psets = psets;

    { // sources
        {
            Many array = list_to_array(execsources_list);

            testbed.sources.all.number = array.number;
            testbed.sources.all._ = calloc(testbed.sources.all.number, sizeof(Source));

            execsources = (struct ExecSource **) array._;
        }

        INITMANY(testbed.sources.binary[0], sources_count.binary[0], Source);
        INITMANY(testbed.sources.binary[1], sources_count.binary[1], Source);
        DGAFOR(cl) {
            INITMANY(testbed.sources.multi[cl], sources_count.multi[cl], Source);
        }

        struct Index index;
        memset(&index, 0, sizeof(struct Index));
        for (int32_t s = 0; s < testbed.sources.all.number; s++) {
            RSource rsource = &testbed.sources.all._[s];
            memcpy(&testbed.sources.all._[s], execsources[s]->source, sizeof(Source));
            testbed.sources.binary[rsource->binaryclass]._[index.binary[rsource->binaryclass]++] = rsource;
            testbed.sources.multi[rsource->dgaclass]._[index.multi[rsource->dgaclass]++] = rsource;
            free(execsources[s]->source);
        }
    }
    
    struct Index wcount[psets.number];
    memset(&wcount, 0, psets.number * sizeof(struct Index));

    { // windowings

        for (int32_t p = 0; p < psets.number; p++) {
            INITMANY(testbed.applies[p].windowings.all, testbed.sources.all.number, Windowing);
            INITMANY(testbed.applies[p].windowings.binary[0], testbed.sources.binary[0].number, Windowing);
            INITMANY(testbed.applies[p].windowings.binary[1], testbed.sources.binary[1].number, Windowing);
            DGAFOR(cl) {
                INITMANY(testbed.applies[p].windowings.multi[cl], testbed.sources.multi[cl].number, Windowing);
            }
        }

        struct Index index[psets.number];
        memset(&index, 0, sizeof(struct Index) * psets.number);
        for (int32_t s = 0; s < testbed.sources.all.number; s++) {
            RSource rsource = &testbed.sources.all._[s];
            const int bc = rsource->binaryclass;
            const int dc = rsource->dgaclass;

            MANY(Windowing) windowings_p = windowing_run_1source_manypsets(rsource, psets, execsources[s]->fn);

            for (int32_t p = 0; p < psets.number; p++) {
                testbed.applies[p].windowings.all._[s] = windowings_p._[p];
                testbed.applies[p].windowings.binary[bc]._[index[p].binary[bc]] = windowings_p._[p];
                testbed.applies[p].windowings.multi[dc]._[index[p].multi[dc]] = windowings_p._[p];

                wcount[p].all += windowings_p._[p].windows.number;
                wcount[p].binary[bc] += windowings_p._[p].windows.number;
                wcount[p].multi[dc] += windowings_p._[p].windows.number;

                index[p].binary[bc] += 1;
                index[p].multi[dc] += 1;
            }

            free(windowings_p._);
        }
    }

    { // windows
        for (int32_t p = 0; p < psets.number; p++) {
            TestBedWindows* tbw = &testbed.applies[p].windows;

            INITMANY(tbw->all, wcount[p].all, Window);
            INITMANY(tbw->binary[0], wcount[p].binary[0], Window);
            INITMANY(tbw->binary[1], wcount[p].binary[1], Window);
            DGAFOR(cl) {
                INITMANY(tbw->multi[cl], wcount[p].multi[cl], Window);
            }

            struct Index index;
            memset(&index, 0, sizeof(struct Index));
            for (int32_t s = 0; s < testbed.applies[p].windowings.all.number; s++) {
                Windowing* windowing = &testbed.applies[p].windowings.all._[s];
                const int bclass = windowing->source->binaryclass;
                const int dclass = windowing->source->dgaclass;

                for (int32_t w = 0; w < windowing->windows.number; w++) {
                    tbw->all._[index.all + w] = &windowing->windows._[w];
                    tbw->binary[bclass]._[index.binary[bclass] + w] = &windowing->windows._[w];
                    tbw->multi[dclass]._[index.multi[dclass] + w] = &windowing->windows._[w];
                }

                index.all += windowing->windows.number;
                index.binary[bclass] += windowing->windows.number;
                index.multi[dclass] += windowing->windows.number;
            }
        }
    }

    free(execsources);
    list_free(execsources_list, 1);

    return testbed;
}

void testbed_free(TestBed* tb) {
    free(tb->sources.all._);
    free(tb->sources.binary[0]._);
    free(tb->sources.binary[1]._);
    DGAFOR(cl) {
        free(tb->sources.multi[cl]._);
    }

    for (int32_t p = 0; p < psets.number; p++) {
        for (int32_t bl = 0; bl < 2; bl++) {
            free(tb->applies[p].windowings.binary[bl]._);
            free(tb->applies[p].windows.binary[bl]._);
        }

        DGAFOR(cl) {
            free(tb->applies[p].windowings.multi[cl]._);
            free(tb->applies[p].windows.multi[cl]._);
        }
        for (int32_t s = 0; s < tb->applies[p].windowings.all.number; s++) {
            free(tb->applies[p].windowings.all._[s].windows._);
        }
        free(tb->applies[p].windowings.all._);
        free(tb->applies[p].windows.all._);
    }
    free(tb->applies);

    free(tb->psets._);
}