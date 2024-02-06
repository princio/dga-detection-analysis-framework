#ifndef __WINDOWINGtb2w_io_H__
#define __WINDOWINGtb2w_io_H__

#include "common.h"

#include "tb2w.h"

// void io_flag(IOReadWrite rw, FILE* file, char* flag_code, int line);
void tb2_io_windows(IOReadWrite rw, FILE* file, RTB2W tb2, MANY(RWindow) windows);

int tb2w_io_windowing(IOReadWrite rw, RTB2W tb2, RWindowing windowing);
int tb2w_io(IOReadWrite rw, char dirname[DIR_MAX], RTB2W* tb2);

#endif