#include "experiment.h"

#include "windowing.h"

WindowingPtr experiment_run(char* rootpath, char*name, WSizes wsizes, PSetGenerator* psetgenerator) {

    WindowingPtr windowing_ptr = NULL;

    windowing_ptr = windowing_load(rootpath, name);

    if (windowing_ptr) return windowing_ptr;

    windowing_ptr = windowing_run(rootpath, name, wsizes, psetgenerator);

    windowing_save(windowing_ptr);

    return windowing_ptr;
}