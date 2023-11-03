
#include "windowing.h"

#include "common.h"
#include "parameters.h"
#include "persister.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

Windowing windowing;

void windowing_message(const DNSMessage message, const MANY(PSet) psets, MANY(WindowingWindows)* sw) {
    for (int32_t p = 0; p < psets.number; p++) {
        
        int wnum;
        PSet* pset;
        Window *window;
        MANY(Window)* windows;

        pset = &psets._[p];
        windows = &sw->_[p].windows;

        wnum = (int) floor(message.fn_req / pset->wsize);

        if (wnum >= windows->number) {
            printf("ERROR\n");
            printf("      wnum: %d\n", wnum);
            printf("     wsize: %d\n", pset->wsize);
            printf("  nwindows: %d\n", windows->number);
        }

        window = &windows->_[wnum];

        if (wnum != 0 && wnum != window->wnum) {
            printf("Error[wnum != window->wnum]:\t%d\t%d\n", wnum, window->wnum);
        }

        int whitelistened = 0;
        double value, logit;

        if (pset->windowing == WINDOWING_Q && message.is_response) {
            return;
        } else
        if (pset->windowing == WINDOWING_R && !message.is_response) {
            return;
        }

        logit = message.logit;
        value = message.value;

        if (message.top10m > 0 && message.top10m < pset->whitelisting.rank) {
            value = 0;
            logit = pset->whitelisting.value;
            whitelistened = 1;
        }
        if (logit == INFINITY) {
            logit = pset->infinite_values.pinf;
        } else
            if (logit == (-1 * INFINITY)) {
            logit = pset->infinite_values.ninf;
        }

        ++window->wcount;
        window->dn_bad_05 += value >= 0.5;
        window->dn_bad_09 += value >= 0.9;
        window->dn_bad_099 += value >= 0.99;
        window->dn_bad_0999 += value >= 0.999;

        window->logit += logit;
        window->whitelistened += whitelistened;
    }
}

void io_windowing_path(const WindowingGalaxy* wg, const Source* source, const PSet *pset, char path[500]) {
    char fname[200];
    char subdigest[9];

    strncpy(subdigest, pset->digest, sizeof subdigest);
    sprintf(fname, "%s_%s_%s", windowing.rootpath, wg->name, source->name, subdigest);
}

int32_t io_windowing(IOReadWrite rw, char path, MANY(Window)* windows) {
    FILE* file;
    int error;

    error = 0;
    file = open_file(rw, path);
    if (file) {
        FRWNPtr fn = rw ? freadN : fwriteN;
        int32_t nw;

        FRW(fn, nw);

        if (nw != windows->number) {
            printf("Warning: nw_read != windows->number [%d != %d]\n", nw, windows->number);
        }

        for (int i = 0; i < windows->number; ++i) {
            Window* window = &windows->_[i];

            FRW(fn, window->source_index);
            FRW(fn, window->pset_index);
            
            FRW(fn, window->wnum);

            FRW(fn, window->dgaclass);
            FRW(fn, window->wcount);
            FRW(fn, window->logit);
            FRW(fn, window->whitelistened);
            FRW(fn, window->dn_bad_05);
            FRW(fn, window->dn_bad_09);
            FRW(fn, window->dn_bad_099);
            FRW(fn, window->dn_bad_0999);
        }

        if (fclose(file)) {
            printf("Error %d\n", __LINE__);
        }
    } else {
        error = 1;
    }

    return error;
}

void windowing_init(PSetGenerator* psetgenerator) {
    windowing.psets = parameters_generate(psetgenerator);
}

WindowingGalaxy* windowing_galaxy_add(char name[50]) {
    WindowingGalaxy* wg = calloc(1, sizeof(WindowingGalaxy));
    sprintf(wg->name, "%s", name);
    list_insert(&windowing.galaxies, wg);
    return wg;
}

WindowingSource* windowing_sources_add(const Source* source, WindowingGalaxy* gl, WindowingFunction fn) {
    WindowingSource* ws_ptr;
    int32_t* sources;
    int32_t* binary;
    int32_t* multi;

    ws_ptr = calloc(1, sizeof(WindowingSource));

    ws_ptr->source = *source;

    INITMANY(ws_ptr->windows, windowing.psets.number, WindowingWindows);
    
    list_insert(&gl->sourceloaders, ws_ptr);

    ++windowing.sources_count.all;
    ++windowing.sources_count.binary[source->dgaclass > 0];
    ++windowing.sources_count.multi[source->dgaclass];

    ws_ptr->fn = fn;

    return ws_ptr;
}

WindowingWindows windowing_windows_make(const Source* source, const PSet* pset) {
    const int32_t nw = N_WINDOWS(source->fnreq_max, pset->wsize);

    WindowingWindows ww;

    ww.windows.number = nw;
    INITMANY(ww.windows, ww.windows.number, Window);
    for (int32_t w = 0; w < ww.windows.number; w++) {
        ww.windows._[w].wnum = w;
    }

    return ww;
}

void windowing_windows_load(WindowingGalaxy* wg, const Source* source, const PSet* pset, WindowingWindows* const ww) {
    char path[500];
    io_windowing_path(wg, source, pset, path);

    if (0 == io_windowing(IO_READ, path, &ww->windows)) {
        ww->loaded = 1;
    }
}

void windowing_windows_save(WindowingGalaxy* wg, const Source* source, const PSet* pset, WindowingWindows* ww) {
    char path[500];

    io_windowing_path(wg, source, pset, path);

    if (io_windowing(IO_WRITE, path, &ww->windows)) {
        ww->saved = 1;
    }
}

WindowingWindows windowing_wsource_load(WindowingGalaxy const * const wg, WindowingSource* const wsource) {
    const Source* const source = &wsource->source;

    for (int32_t p = 0; p < windowing.psets.number; p++) {
        wsource->windows._[p] = windowing_windows_make(source, &windowing.psets._[p]);
        windowing_windows_load(wg, &wsource->source, &windowing.psets._[p], &wsource->windows._[p]);

        wsource->loaded += wsource->windows._[p].loaded;
    }
}

WindowingWindows windowing_wsource_save(WindowingGalaxy const * const wg, WindowingSource* const wsource) {
    const Source* const source = &wsource->source;

    for (int32_t p = 0; p < windowing.psets.number; p++) {
        windowing_windows_save(wg, &wsource->source, &windowing.psets._[p], &wsource->windows._[p]);

        wsource->saved += wsource->windows._[p].saved;
    }
}

void windowing_run() {
    ListItem* wg_cursor = windowing.galaxies.root;
    while(wg_cursor) {
        WindowingGalaxy* wg;
        ListItem* wsource_cursor;

        wg  = wg_cursor->item;
        wsource_cursor = wg->sourceloaders.root;

        while(wsource_cursor) {
            WindowingSource* wsource = wsource_cursor->item;

            INITMANY(wsource->windows, windowing.psets.number, MANY(WindowingWindows));

            windowing_wsource_load(wg, wsource);

            if (wsource->loaded < wsource->windows.number) {
                wsource->fn(&wsource->source, &wsource->windows);
            }

            for (int32_t p = 0; p < windowing.psets.number; p++) {
                WindowingWindows* const ww = &wsource->windows._[p];
                if (!ww->loaded) {
                    windowing_windows_save(wg, wsource, &windowing.psets._[p], ww);
                }
            }

            wsource_cursor = wsource_cursor->next;
        }

        wg_cursor = wg_cursor->next;
    }
}

void windowing_perform(const WindowingSource* wsource, const MANY(WindowingWindows) wws, const int32_t row) {

}

Experiment windowing_experiment() {

    Experiment exp;

    exp.psets = windowing.psets;

    struct {
        int32_t galaxies;
        int32_t all;
        int32_t binary[2];
        int32_t multi[N_DGACLASSES];
    } index;

    struct {
        int32_t all;
        int32_t binary[2];
        int32_t multi[N_DGACLASSES];
    } windowing_index;

    INITMANY(exp.galaxies, windowing.galaxies.size, Galaxy);

    INITMANY(exp.sources.all, windowing.sources_count.all, Source);
    INITMANY(exp.sources.binary[0], windowing.sources_count.binary[0], RSource);
    INITMANY(exp.sources.binary[1], windowing.sources_count.binary[1], RSource);
    INITMANY(exp.sources.multi[0], windowing.sources_count.multi[0], RSource);
    INITMANY(exp.sources.multi[1], windowing.sources_count.multi[1], RSource);
    INITMANY(exp.sources.multi[2], windowing.sources_count.multi[2], RSource);

    INITMANY(exp.windowings.all, windowing.psets.number * windowing.sources_count.all, Windowing2);
    INITMANY(exp.windowings.binary[0], windowing.psets.number * windowing.sources_count.binary[0], RWindowing2);
    INITMANY(exp.windowings.binary[1], windowing.psets.number * windowing.sources_count.binary[1], RWindowing2);
    INITMANY(exp.windowings.multi[0], windowing.psets.number * windowing.sources_count.multi[0], RWindowing2);
    INITMANY(exp.windowings.multi[1], windowing.psets.number * windowing.sources_count.multi[1], RWindowing2);
    INITMANY(exp.windowings.multi[2], windowing.psets.number * windowing.sources_count.multi[2], RWindowing2);

    
    memset(&index, 0, sizeof(index));
    memset(&windowing_index, 0, sizeof(windowing_index));

    ListItem* wg_cursor = windowing.galaxies.root;
    while(wg_cursor) {
        WindowingGalaxy* const wg  = wg_cursor->item;
        Galaxy* const galaxy = &exp.galaxies._[index.galaxies++];

        ListItem* wsource_cursor = wg->sourceloaders.root;

        {
            strcpy(galaxy->name, wg->name);
        }

        while(wsource_cursor) {
            WindowingSource* wsource = wsource_cursor->item;
            Source* const source = &exp.sources.all._[index.all];

            exp.sources.binary[DGABINARY(source->dgaclass)]._[index.binary[DGABINARY(source->dgaclass)]] = source;
            exp.sources.multi[source->dgaclass]._[index.multi[source->dgaclass]] = source;

            source->index.all = index.all;
            source->index.binary = index.binary[DGABINARY(source->dgaclass)];
            source->index.multi = index.multi[source->dgaclass];

            source->index.galaxy = index.galaxies;

            for (int32_t p = 0; p < windowing.psets.number; p++) {
                WindowingWindows* ww = &wsource->windows._[p];
                Windowing2* const windowing2 = &exp.windowings.all._[windowing_index.all];

                exp.windowings.binary[DGABINARY(source->dgaclass)]._[index.binary[DGABINARY(source->dgaclass)]] = windowing2;
                exp.windowings.multi[source->dgaclass]._[index.binary[source->dgaclass]] = windowing2;

                for (int32_t w = 0; w < windowing2->number; w++) {
                    windowing2->_[w].source_index = index.all;
                    windowing2->_[w].pset_index = p;
                }
            }

            wsource_cursor = wsource_cursor->next;
        }

        wg_cursor = wg_cursor->next;
    }
}