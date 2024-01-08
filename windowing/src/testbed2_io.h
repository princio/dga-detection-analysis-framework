#ifndef __WINDOWINGTESTBED2_IO_H__
#define __WINDOWINGTESTBED2_IO_H__

#include "common.h"
#include "testbed2.h"

int testbed2_io_windowing_applied(RTestBed2 tb2, RWindowing windowing, size_t idxconfig);
int testbed2_io_windowing_apply_windows_file(IOReadWrite rw, RTestBed2 tb2, RWindowing windowing, size_t idxconfig);
void testbed2_io_dataset(IOReadWrite rw, FILE* file, RTestBed2 tb2, const size_t idxwsize, const size_t idxtry);
int testbed2_io(IOReadWrite rw, char dirname[200], RTestBed2* tb2);

#endif