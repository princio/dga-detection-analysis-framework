

#ifndef __TESTER_H__
#define __TESTER_H__

#include "dn.h"

#include <stdio.h>

void tester_init();

void tester_close();

void tester_fprintf(const char * fmt, ...);

void tester_stratosphere_hex(int trys, int capture_index, int wnum, int ws, int id, int r, WindowMetricSets metrics);
void tester_stratosphere(int trys, int capture_index, int wnum, int ws, int id, int r, WindowMetricSets metrics);

void tester_comparer(int capture_index, int wn, int ws, WindowMetricSets metrics0, WindowMetricSets metrics1);

#endif