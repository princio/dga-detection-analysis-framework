
#include "testbed.h"

#include "cache.h"
#include "io.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

struct ExecSource {
    Index index;
    Source* source;
    SourceWindowingExecution type;
    WindowingAPFunction fn;
};

MANY(PSet) psets;
List galaxies;
List execsources_list;
Index sources_count;

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
    exec_source->index = sources_count;

    source->index = sources_count; 

    list_insert(&execsources_list, exec_source);

    ++sources_count.all;
    ++sources_count.binary[source->dgaclass > 0];
    ++sources_count.multi[source->dgaclass];
}

void _testbed_run(TestBed* testbed) {

    testbed->applies = calloc(psets.number, sizeof(TestBedPSetApply));
    testbed->psets = psets;

    // sources
    struct ExecSource **execsources;
    Many array = list_to_array(execsources_list);
    execsources = (struct ExecSource **) array._;

    testbed->sources.all.number = array.number;
    testbed->sources.all._ = calloc(testbed->sources.all.number, sizeof(Source));

    for (int32_t s = 0; s < testbed->sources.all.number; s++) {
        memcpy(&testbed->sources.all._[s], execsources[s]->source, sizeof(Source));
        free(execsources[s]->source);
    }

    // windowings
    for (int32_t p = 0; p < psets.number; p++) {
        INITMANY(testbed->applies[p].windowings.all, testbed->sources.all.number, Windowing);
    }


    for (int32_t s = 0; s < testbed->sources.all.number; s++) {
        RSource rsource = &testbed->sources.all._[s];

        MANY(Windowing) windowings_p = windowing_run_1source_manypsets(rsource, psets, execsources[s]->fn);

        for (int32_t p = 0; p < psets.number; p++) {
            testbed->applies[p].windowings.all._[s] = windowings_p._[p];
        }

        free(windowings_p._);
    }

    free(execsources);
    list_free(execsources_list, 1);
}

void _testbed_fill(TestBed* testbed) {
    struct Index count_windows_perpset[psets.number];
    struct Index count_sources;
    struct Index index;

    { // sources
        memset(&count_sources, 0, sizeof(struct Index));

        for (int32_t s = 0; s < testbed->sources.all.number; s++) {
            count_sources.all++;
            count_sources.binary[testbed->sources.all._[s].binaryclass]++;
            count_sources.multi[testbed->sources.all._[s].dgaclass]++;
        }

        BINFOR(bn) INITMANY(testbed->sources.binary[bn], count_sources.binary[bn], Source);
        DGAFOR(cl) INITMANY(testbed->sources.multi[cl], count_sources.multi[cl], Source);

        memset(&index, 0, sizeof(struct Index));
        for (int32_t s = 0; s < testbed->sources.all.number; s++) {
            TCPC(Source) rsource = &testbed->sources.all._[s];
            testbed->sources.binary[rsource->binaryclass]._[index.binary[rsource->binaryclass]++] = rsource;
            testbed->sources.multi[rsource->dgaclass]._[index.multi[rsource->dgaclass]++] = rsource;
        }
    }

    memset(&count_windows_perpset, 0, psets.number * sizeof(struct Index));
    for (int32_t p = 0; p < psets.number; p++) {
        TestBedWindowing* testbedwindowings = &testbed->applies[p].windowings;

        BINFOR(bn) INITMANY(testbedwindowings->binary[bn], count_sources.binary[bn], Windowing);
        DGAFOR(cl) INITMANY(testbedwindowings->multi[cl], count_sources.multi[cl], Windowing);

        memset(&index, 0, sizeof(struct Index));
        for (int32_t s = 0; s < testbed->sources.all.number; s++) {
            TCPC(Source) rsource = &testbed->sources.all._[s];
            TCPC(Windowing) windowing = &testbedwindowings->all._[s];

            const int bc = rsource->binaryclass;
            const int mc = rsource->dgaclass;

            int32_t * const bindex = &index.binary[bc];
            int32_t * const mindex = &index.multi[mc];

            testbed->sources.binary[bc]._[*bindex] = rsource;
            testbed->sources.multi[mc]._[*mindex] = rsource;

            testbedwindowings->binary[bc]._[*bindex] = *windowing;
            testbedwindowings->multi[mc]._[*mindex] = *windowing;

            (*bindex)++;
            (*mindex)++;

            count_windows_perpset[p].all += windowing->windows.number;
            count_windows_perpset[p].binary[bc] += windowing->windows.number;
            count_windows_perpset[p].multi[mc] += windowing->windows.number;
        }

        INITMANY(testbed->applies[p].windows.all, count_windows_perpset[p].all, RWindow);
        BINFOR(bn) INITMANY(testbed->applies[p].windows.binary[bn], count_windows_perpset[p].binary[bn], RWindow);
        DGAFOR(cl) INITMANY(testbed->applies[p].windows.multi[cl], count_windows_perpset[p].multi[cl], RWindow);
        
        TestBedWindows* tbw = &testbed->applies[p].windows;

        memset(&index, 0, sizeof(struct Index));
        for (int32_t s = 0; s < testbedwindowings->all.number; s++) {
            Windowing* windowing = &testbedwindowings->all._[s];

            const int bclass = windowing->source->binaryclass;
            const int dclass = windowing->source->dgaclass;

            Index const source_index = testbed->sources.all._[s].index;
            for (int32_t w = 0; w < windowing->windows.number; w++) {
                windowing->windows._[w].source_index = source_index;

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

TestBed* testbed_run() {
    TestBed* testbed = calloc(1, sizeof(TestBed));
    
    _testbed_run(testbed);

    _testbed_fill(testbed);

    char objid[IO_OBJECTID_LENGTH];
    testbed_io_objid(testbed, objid);
    cache_settestbed(objid);

    return testbed;
}

void testbed_free(TestBed* tb) {
    free(tb->sources.all._);
    free(tb->sources.binary[0]._);
    free(tb->sources.binary[1]._);
    DGAFOR(cl) {
        free(tb->sources.multi[cl]._);
    }

    for (int32_t p = 0; p < tb->psets.number; p++) {
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
    free(psets._);
    free(tb);
}

void testbed_io_objid(TCPC(void) obj, char objid[IO_OBJECTID_LENGTH]) {
    TCPC(TestBed) tb = obj;

    memset(objid, 0, IO_OBJECTID_LENGTH);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(objid, "testbed_%d_%d_%02d%02d%02d_%02d%02d%02d", tb->psets.number, tb->sources.all.number, (tm.tm_year + 1900) - 2000, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void testbed_io(IOReadWrite rw, FILE* file, void* obj) {
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
            INITMANY(tb->applies[p].windowings.all, tb->sources.all.number, TestBedWindowing);
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

int testbed_save(TestBed* tb) {
    return io_save(tb, 0, testbed_io_objid, testbed_io);
}

TestBed* testbed_load(char* objid) {
    char dirpath[500];
    strcpy(dirpath, ROOT_PATH);
    sprintf(dirpath + strlen(dirpath), "/%s", objid);

    if(!io_direxists(dirpath)) {
        fprintf(stderr, "Error directory %s not exists", dirpath);
        return NULL;
    }

    cache_settestbed(objid);

    TestBed *tb = calloc(1, sizeof(TestBed));

    if(io_load(objid, 0, testbed_io, tb)) {
        return NULL;
    }

    _testbed_fill(tb);

    return tb;
}

void testbed_io_txt(TCPC(void) obj) {
    char objid[IO_OBJECTID_LENGTH];
    TCPC(TestBed) tb = obj;
    char fname[700];
    FILE* file;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    testbed_io_objid(obj, objid);

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