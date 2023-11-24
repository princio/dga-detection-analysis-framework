
#include "testbed2.h"

#include "cache.h"
#include "io.h"
#include "tetra.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

TestBed2* testbed2_create(MANY(WSize) wsizes) {
    TestBed2* tb2 = calloc(1, sizeof(TestBed2));

    INITMANY(tb2->wsizes, wsizes.number, MANY(WSize));
    INITMANY(tb2->datasets, wsizes.number, MANY(Dataset0));

    for (int32_t i = 0; i < tb2->wsizes.number; i++) {
        tb2->datasets._[i].wsize = wsizes._[i];
        tb2->wsizes._[i] = wsizes._[i];
    }

    return tb2;
}

int32_t _testbed2_get_index(MANY(RSource) sources) {
    int32_t index;
    for (index = 0; index < sources.number; ++index) {
        if (sources._[index] == 0x0) break;
    }
    return index;
}

void testbed2_source_add(TestBed2* tb2, __Source* source) {
    sources_tetra_add(&tb2->sources, source);
}

void testbed2_run(TestBed2* tb2) {
    tetra_apply_all((Tetra*) &tb2->sources, (void(*)(void*)) sources_finalize);

    tetra_init_with_tetra((Tetra*) &tb2->windowingss, (Tetra*) &tb2->sources, sizeof(MANY(MANY(RWindowing))));

    TetraX counter[tb2->wsizes.number]; memset(counter, 0, tb2->wsizes.number * sizeof(Index));
    TetraX sindex; memset(&sindex, 0, sizeof(TetraX));
    for (int32_t s = 0; s < tb2->sources.all.number; s++) {
        RSource source = tb2->sources.all._[s];
        TetraItem titem;

        titem = tetra_get((Tetra*) &tb2->windowingss, sindex, source->wclass);

        INITMANYREF(titem.all, tb2->wsizes.number, RWindowing);
        INITMANYREF(titem.binary, tb2->wsizes.number, RWindowing);
        INITMANYREF(titem.multi, tb2->wsizes.number, RWindowing);

        for (int32_t u = 0; u < tb2->wsizes.number; u++) {
            const WSize wsize = tb2->wsizes._[u];
            RWindowing windowing = windowings_alloc(source, wsize);
    
            titem.all->_[u] = windowing;
            titem.binary->_[u] = windowing;
            titem.multi->_[u] = windowing;

            windowing_run(windowing);

            tetra_increment(&sindex, source->wclass, 1);
            tetra_increment(&counter[u], source->wclass, windowing->windows.number);
            printf("%d\n", windowing->windows.number);
        }
    }

    TetraX index; memset(&index, 0, sizeof(TetraX));
    for (int32_t u = 0; u < tb2->wsizes.number; u++) {
        TETRA(MANY(RWindow0))* dataset = &tb2->datasets._[u].windows;

        tetra_init((Tetra*) dataset, counter[u], sizeof(RWindow0));

        TetraX sindex; memset(&sindex, 0, sizeof(TetraX));
        for (int32_t s = 0; s < tb2->sources.all.number; s++) {
            RWindowing windowing = tb2->windowingss.all._[s]._[u];
            RSource source = tb2->sources.all._[s];

            TetraItem titem = tetra_get((Tetra*) dataset, sindex, source->wclass);

            memcpy(titem.all, windowing->windows._, windowing->windows.number * sizeof(RWindow0));
            memcpy(titem.binary, windowing->windows._, windowing->windows.number * sizeof(RWindow0));
            memcpy(titem.multi, windowing->windows._, windowing->windows.number * sizeof(RWindow0));

            tetra_increment(&sindex, source->wclass, windowing->windows.number);
        }
    }
}

void testbed2_free(TestBed2* tb) {
    tetra_free((Tetra*) &tb->sources);
    TetraX sindex; memset(&sindex, 0, sizeof(TetraX));
    for (int32_t s = 0; s < tb->sources.all.number; s++) {
        const RSource source = tb->sources.all._[s];
        for (int32_t u = 0; u < tb->wsizes.number; u++) {
            TetraItem ti = tetra_get((Tetra*) &tb->windowingss, sindex, source->wclass);
            free(ti.all->_);
            free(ti.binary->_);
            free(ti.multi->_);
            tetra_increment(&sindex, source->wclass, 1);
        }
    }

    for (int32_t u = 0; u < tb->datasets.number; u++) {
        tetra_free((Tetra*) &tb->datasets._[u]);
    }
    
    tetra_free((Tetra*) &tb->windowingss);
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
    for (int32_t p = 0; p < tb->psets.number; p++) {
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
    for (int32_t s = 0; s < tb->sources.all.number; s++) {
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

    for (int32_t p = 0; p < tb->psets.number; p++) {

        if (rw == IO_READ) {
            INITMANY(tb->applies[p].windowings.all, tb->sources.all.number, TestBedSourceApplies);
        }
        for (int32_t s = 0; s < tb->sources.all.number; s++) {
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

    for (int32_t p = 0; p < tb->psets.number; p++) {
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
    
    for (int32_t s = 0; s < tb->sources.all.number; s++) {
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