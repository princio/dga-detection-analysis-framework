#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include "kfold.h"
#include "parameters.h"
#include "sources.h"

// typedef struct GalaxyLoader {
//     int32_t index;

//     int loaded;

//     List sourceloaders;
//     Galaxy galaxy;
// } GalaxyLoader;

// MAKEMANY(GalaxyLoader);

// typedef struct SourceLoader {
//     int32_t index;
//     int initilized;
//     int loaded;
//     Source source;
// } SourceLoader;

// MAKEMANY(SourceLoader);

// typedef struct ExperimentLoader {
//     struct {
//         int32_t all;
//         int32_t binary[2];
//         int32_t multi[N_DGACLASSES];

//         List list;
//     } sources;

//     List galaxies;
// } ExperimentLoader;


typedef struct ExperimentWindowing {
    MANY(Windowing2) all;
    MANY(RWindowing2) binary[2];
    DGAMANY(RWindowing2) multi;
} ExperimentWindowing;

typedef struct Experiment {
    char name[100];
    char rootpath[500];

    MANY(PSet) psets;

    MANY(Galaxy) galaxies;
    
    struct {
        MANY(Source) all;
        MANY(RSource) binary[2];
        DGAMANY(RSource) multi;
    } sources;

    struct {
        MANY(Windowing2) all;
        MANY(RWindowing2) binary[2];
        DGAMANY(RWindowing2) multi;
    } windowings;
} Experiment;

extern Experiment experiment;

void experiment_init();

void experiment_set_name(char[100]);
void experiment_set_path(char[500]);
void experiment_set_psets(MANY(PSet) psets);
MANY(Window) experiment_windows_init(const Source*, const PSet*);

WindowingWindows experiment_windowing_load(const Galaxy*, const Source*, const PSet*);
void experiment_sourcewindowing_save(const Galaxy* galaxy, const Source* source, const PSet* pset, WindowingWindows sw);

void experiment_load();

int experiment_loaded();

#endif