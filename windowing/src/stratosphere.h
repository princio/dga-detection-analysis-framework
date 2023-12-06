
#ifndef __STRATOSPHERE_H__
#define __STRATOSPHERE_H__

#include "testbed2.h"

#include "windows.h"

void stratosphere_add(RTestBed2 testbed);
void stratosphere_apply(MANY(RWindowing) windowings, TCPC(MANY(PSet)) psets, int32_t loaded[]);

#endif