
#include "experiment.h"

#include "common.h"
#include "parameters.h"
#include "persister.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

ExperimentLoader experiment_loader;
Experiment experiment;
int LOADED = 0;

void experiment_init() {
    memset(&experiment, 0, sizeof(Experiment));
    memset(&experiment_loader, 0, sizeof(ExperimentLoader));
}
void experiment_set_name(char name[100]) {
    if (strlen(name) > 100) {
        fprintf(stderr, "Error experiment set name: %d > %d", strlen(name), 100);
        exit(1);
    }
    sprintf(experiment.name, "%s", name);
}

void experiment_set_path(char path[500]) {
    if (strlen(path) > 500) {
        fprintf(stderr, "Error experiment set name: %d > %d", strlen(path), 500);
        exit(1);
    }
    sprintf(experiment.rootpath, "%s", path);
}

void experiment_set_psets(MANY(PSet) psets) {
    experiment.psets = psets;
}

GalaxyLoader* experiment_galaxy_add(char name[50]) {
    GalaxyLoader* gl = calloc(1, sizeof(GalaxyLoader));
    sprintf(gl->galaxy.name, "%s", name);
    list_insert(&experiment_loader.galaxies, gl);
    return gl;
}

Source* experiment_source_load(char name[100]) {
    persister_source(PERSITER_READ, name);
}

void experiment_sources_add(GalaxyLoader* gl, const Source* source) {
    SourceLoader* sourceloader_ptr;
    GalaxyLoader* gl;
    int32_t* sources;
    int32_t* binary;
    int32_t* multi;

    sourceloader_ptr = calloc(1, sizeof(SourceLoader));
    memcpy(sourceloader_ptr, source, sizeof(SourceLoader));

    sourceloader_ptr->source = *source;
    
    list_insert(&gl->sourceloaders, sourceloader_ptr);
    
    // sources = &experiment_loader.sources.all;
    // binary = &experiment_loader.sources.binary[source->dgaclass > 0];
    // multi = &experiment_loader.sources.multi[source->dgaclass];

    // sourceloader_ptr->source.index.galaxy = gl->dgalist[source->dgaclass].size;
    // sourceloader_ptr->source.index.total = (*sources)++;
    // sourceloader_ptr->source.index.binary = (*binary)++;
    // sourceloader_ptr->source.index.multi = (*multi)++;    
}

MANY(Window) experiment_windows_init(const Source* source, const PSet* pset) {
    MANY(Window) windows;
    int32_t nw;

    nw = N_WINDOWS(source->fnreq_max, pset->wsize);

    INITMANY(windows, nw, Window);

    for (int32_t w = 0; w < nw; w++) {
        windows._[w].source_index = source->index;
        windows._[w].wnum = w;
        windows._[w].pset_id = pset->id;
    }

    return windows;
}

WindowingWindows experiment_load(const Galaxy* galaxy, const Source* source, const PSet* pset) {
    WindowingWindows sw;
    memset(&sw, 0, sizeof(WindowingWindows));

    sw.loaded = 0 == persister_windowing(PERSITER_READ, galaxy, source, pset, &sw.windows);

    if (!sw.loaded) {
        sw.windows = experiment_windows_init(source, pset);
    }

    return sw;
}

void experiment_sourcewindowing_save(const Galaxy* galaxy, const Source* source, const PSet* pset, WindowingWindows sw) {
    int ret = persister_windowing(PERSITER_WRITE, galaxy, source, pset, &sw.windows);

    if (ret) {
        printf("Failed to save windowing.");
    }
}

void experiment_load(char name[50], char rootpath[100]) {
    
}
