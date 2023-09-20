#ifndef __PERSISTER_H__
#define __PERSISTER_H__

#include "dn.h"

int persister_write__psets(WindowingPtr windowing);
int persister_read__psets(WindowingPtr windowing);


int persister_write__captures(WindowingPtr windowing);
int persister_read__captures(WindowingPtr windowing);


int persister_write__capturewsets(WindowingPtr windowing, int32_t capture_index);
int persister_read__capturewsets(WindowingPtr windowing, int32_t capture_index);


void persister_test();

#endif