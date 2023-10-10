
#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "dn.h"

void windowing_capture_init(WindowingPtr windowing, int capture_index);
void windowing_captures_init(WindowingPtr windowing);
void windowing_fetch(WindowingPtr windowing);

int windowing_compare(WindowingPtr w1, WindowingPtr w2, int);

WindowingPtr windowing_run(char* rootpath, char* name, WSizes wsizes, PSetGenerator* psetgenerator);

void windowing_description(WindowingPtr windowing);

int windowing_save(WindowingPtr windowing_ptr);

WindowingPtr windowing_load(char* rootpath, char* name);

#endif