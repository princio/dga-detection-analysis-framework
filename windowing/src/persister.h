#ifndef __PERSISTER_H__
#define __PERSISTER_H__

#include "dn.h"

int persister_write__psets(char path[], PSets pis, const int N_PIS);
int persister_read__psets(char path[], PSets* pis, int* N_PIS);


int persister_write__captures(char path[], WindowingPtr windowing);
int persister_read__captures(char path[], WindowingPtr windowing);


int persister_write__capturewindowing(char* rootpath, WindowingPtr windowing, int32_t capture_index);
int persister_read__capturewindowing(char* rootpath, WindowingPtr windowing, int32_t capture_index);


void persister_test();

#endif