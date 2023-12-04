
#include "testbed2.h"

#include "cache.h"
#include "dataset.h"
#include "io.h"
#include "stratosphere.h"
#include "tetra.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

TestBed2* testbed2_create(MANY(WSize) wsizes) {
    TestBed2* tb2 = calloc(1, sizeof(TestBed2));

    INITMANY(tb2->wsizes, wsizes.number, WSize);
    INITMANY(tb2->datasets.wsize, wsizes.number, RDataset0);

    for (size_t i = 0; i < tb2->wsizes.number; i++) {
        tb2->wsizes._[i] = wsizes._[i];
    }

    return tb2;
}

size_t _testbed2_get_index(MANY(RSource) sources) {
    size_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
}

void testbed2_source_add(TestBed2* tb2, __Source* source) {
    sources_add(&tb2->sources, source);
}

void testbed2_windowing(TestBed2* tb2) {
    sources_finalize(&tb2->sources);

    INITMANY(tb2->windowings.source, tb2->sources.number, TB2WindowingsSource);

    int32_t count_windows[tb2->wsizes.number][N_DGACLASSES];
    memset(&count_windows, 0, sizeof(int32_t) * tb2->wsizes.number * N_DGACLASSES);
    for (size_t s = 0; s < tb2->sources.number; s++) {
        RSource source = tb2->sources._[s];
        MANY(RWindowing)* windowings = &tb2->windowings.source._[s].wsize;

        INITMANYREF(windowings, tb2->wsizes.number, RWindowing);

        for (size_t u = 0; u < tb2->wsizes.number; u++) {
            const WSize wsize = tb2->wsizes._[u];
            windowings->_[u] = windowings_create(source, wsize);

            count_windows[u][source->wclass.mc] += windowings->_[u]->windows.number;
        }
    }

    for (size_t u = 0; u < tb2->wsizes.number; u++) {
        RDataset0* dataset = &tb2->datasets.wsize._[u];

        MANY(RWindowing) tmp;

        INITMANY(tmp, tb2->sources.number, RWindowing);

        for (size_t s = 0; s < tb2->sources.number; s++) {
            tmp._[s] = tb2->windowings.source._[s].wsize._[u];
        }

        tb2->datasets.wsize._[u] = dataset0_from_windowing(tmp, tb2->psets.number);

        FREEMANY(tmp);
    }
}

void testbed2_apply(TestBed2* tb2, const MANY(PSet) psets) {
    for (size_t s = 0; s < tb2->sources.number; s++) {
        TB2WindowingsSource swindowings = tb2->windowings.source._[s];
        for (size_t i = 0; i < tb2->wsizes.number; i++) {
            for (size_t w = 0; w < swindowings.wsize._[i]->windows.number; w++) {
                INITMANY(swindowings.wsize._[i]->windows._[w]->applies, psets.number, WApply);
            }
        }
        stratosphere_apply(swindowings.wsize, psets, NULL);
    }

    tb2->applied = 1;
    tb2->psets = psets;
}

void testbed2_free(TestBed2* tb2) {
    for (size_t s = 0; s < tb2->sources.number; s++) {
        FREEMANY(tb2->windowings.source._[s].wsize);
    }
    FREEMANY(tb2->windowings.source);
    FREEMANY(tb2->datasets.wsize);
    FREEMANY(tb2->sources);
    FREEMANY(tb2->wsizes);
    free(tb2);
}

/*
void testbed2_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    TCPC(TestBed) tb = obj;

    memset(objid, 0, IO_OBJECTID_LENGTH);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(objid, "testbed2_%d_%d_%02d%02d%02d_%02d%02d%02d", tb->psets.number, tb->sources.all.number, (tm.tm_year + 1900) - 2000, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void testbed2_io(IOReadWrite rw, FILE* file, void* obj) {
    TestBed* tb = obj;

    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    FRW(tb->psets.number);
    if (rw == IO_READ) {
        INITMANY(tb->psets, tb->psets.number, PSet);
    }
    for (size_t p = 0; p < tb->psets.number; p++) {
        char objid[IO_OBJECTID_LENGTH];
        memset(objid, 0, IO_OBJECTID_LENGTH);

        if (rw == IO_WRITE) {
            parameters_io_objid(&tb->psets._[p], objid);
        }

        FRW(objid);

        if (rw == IO_WRITE) {
            io_save(&tb->psets._[p], 1, parameters_io_objid, parameters_io);
        } else {
            io_load(objid, 1, parameters_io, &tb->psets._[p]);
        }
    }
    FRW(tb->sources.all.number);
    if (rw == IO_READ) {
        INITMANY(tb->sources.all, tb->sources.all.number, Source);
    }
    for (size_t s = 0; s < tb->sources.all.number; s++) {
        char objid[IO_OBJECTID_LENGTH];
        memset(objid, 0, IO_OBJECTID_LENGTH);

        if (rw == IO_WRITE) {
            sources_io_objid(&tb->sources.all._[s], objid);
        }
        
        FRW(objid);

        if (rw == IO_WRITE) {
            io_save(&tb->sources.all._[s], 1, sources_io_objid, sources_io);
        } else {
            io_load(objid, 1, sources_io, &tb->sources.all._[s]);
        }
    }

    if (rw == IO_READ) {
        tb->applies = calloc(tb->psets.number, sizeof(TestBedPSetApply));
    }

    for (size_t p = 0; p < tb->psets.number; p++) {

        if (rw == IO_READ) {
            INITMANY(tb->applies[p].windowings.all, tb->sources.all.number, TestBedSourceApplies);
        }
        for (size_t s = 0; s < tb->sources.all.number; s++) {
            Windowing* windowing = &tb->applies[p].windowings.all._[s];

            char objid[IO_OBJECTID_LENGTH];
            memset(objid, 0, IO_OBJECTID_LENGTH);

            if (rw == IO_WRITE) {
                windowing_io_objid(windowing, objid);
            }

            FRW(objid);

            if (rw == IO_WRITE) {
                io_save(windowing, 1, windowing_io_objid, windowing_io);
            } else {
                windowing_init(&tb->sources.all._[s], &tb->psets._[p], windowing);
                io_load(objid, 1, windowing_io, windowing);
            }
        }
    }
}

int testbed2_save(TestBed* tb) {
    return io_save(tb, 0, testbed2_io_objid, testbed2_io);
}

TestBed* testbed2_load(char* objid) {
    char dirpath[500];
    strcpy(dirpath, ROOT_PATH);
    sprintf(dirpath + strlen(dirpath), "/%s", objid);

    if(!io_direxists(dirpath)) {
        fprintf(stderr, "Error directory %s not exists", dirpath);
        return NULL;
    }

    cache_settestbed(objid);

    TestBed *tb = calloc(1, sizeof(TestBed));

    if(io_load(objid, 0, testbed2_io, tb)) {
        return NULL;
    }

    _testbed2_fill(tb);

    return tb;
}

void testbed2_io_txt(TCPC(void) obj) {
    char objid[IO_OBJECTID_LENGTH];
    TCPC(TestBed) tb = obj;
    char fname[700];
    FILE* file;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    testbed2_io_objid(obj, objid);

    sprintf(fname, "%s/testbed.md", ROOT_PATH);

    file = io_openfile(IO_WRITE, fname);

    fprintf(file, "# TestBed %s\n\n", objid);

    fprintf(file, "Run at: %04d/%02d/%02d %02d:%02d:%02d\n\n", (tm.tm_year + 1900), tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    fprintf(file, "## Parameters %d\n\n", tb->psets.number);

    fprintf(file, "%8s    ", "nn");
    fprintf(file, "%6s    ", "wsize");
    fprintf(file, "%15s   ", "infinite values");
    fprintf(file, "%13s\n", "whitelisting");

    for (size_t p = 0; p < tb->psets.number; p++) {
        TCPC(PSet) pset = &tb->psets._[p];

        fprintf(file, "%8s    ", NN_NAMES[pset->nn]);
        fprintf(file, "%6d    ", pset->wsize);
        fprintf(file, "%-6.2f %6.2f      ", pset->infinite_values.ninf, pset->infinite_values.pinf);
        fprintf(file, "%-6d %6.2f\n", pset->whitelisting.rank, pset->whitelisting.value);
    }


    fprintf(file, "\n\n## Sources %d\n\n", tb->sources.all.number);

    fprintf(file, "%8s    ", "bclass");
    fprintf(file, "%8s    ", "mclass");
    fprintf(file, "%10s    ", "qr");
    fprintf(file, "%10s    ", "q");
    fprintf(file, "%10s    ", "r");
    fprintf(file, "%10s    ", "fnreq_max");
    fprintf(file, "%s\n", "name");
    
    for (size_t s = 0; s < tb->sources.all.number; s++) {
        TCPC(Source) source = &tb->sources.all._[s];
        fprintf(file, "%8d    ", source->binaryclass);
        fprintf(file, "%8d    ", source->dgaclass);
        fprintf(file, "%10ld    ", source->qr);
        fprintf(file, "%10ld    ", source->q);
        fprintf(file, "%10ld    ", source->r);
        fprintf(file, "%10ld    ", source->fnreq_max);
        fprintf(file, "%s\n", source->name);
    }

    fclose(file);
}
*/