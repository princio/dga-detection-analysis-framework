
#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include "dn.h"

void windowing_capture_init(WindowingPtr windowing, int capture_index);
void windowing_captures_init(WindowingPtr windowing);
void windowing_fetch(WindowingPtr windowing);

int windowing_compare(WindowingPtr w1, WindowingPtr w2, int);

#endif