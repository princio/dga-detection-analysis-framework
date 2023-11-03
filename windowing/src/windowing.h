#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "kfold.h"
#include "parameters.h"
#include "sources.h"

MAKEDGA(List);

typedef struct WindowingWindows {
    int loaded;
    int initilized;
    int saved;

    MANY(Window) windows;

} WindowingWindows;

MAKEMANY(WindowingWindows);

typedef void (*WindowingFunction)(const Source*, MANY(WindowingWindows)* const);

typedef struct WindowingGalaxy {
    int32_t index;

    char name[50];

    int loaded;

    List sourceloaders;
} WindowingGalaxy;

MAKEMANY(WindowingGalaxy);

typedef struct WindowingSource {
    int32_t index;

    WindowingGalaxy* galaxy;

    int loaded;
    int saved;

    WindowingFunction fn;
    
    MANY(WindowingWindows) windows;
    
    Source source;
} WindowingSource;

MAKEMANY(WindowingSource);

typedef struct Windowing {
    char rootpath[300];
    struct {
        int32_t all;
        int32_t binary[2];
        int32_t multi[N_DGACLASSES];
    } sources_count;
    
    MANY(PSet) psets;
    List galaxies;
} Windowing;

typedef MANY(Window) Windowing2;
MAKEMANY(Windowing2);

typedef MANY(Window)* RWindowing2;
MAKEMANY(RWindowing2);
MAKEDGAMANY(RWindowing2);

extern Windowing windowing;

void windowing_message(const DNSMessage message, const MANY(PSet) psets, MANY(WindowingWindows)* sw);

void windowing_init(PSetGenerator* psetgenerator);

WindowingGalaxy* windowing_galaxy_add(char[50]);

WindowingSource* windowing_sources_add(const Source*, WindowingGalaxy*, WindowingFunction);

WindowingWindows windowing_galaxy_windows_load(const WindowingGalaxy*);

void windowing_save();

#endif