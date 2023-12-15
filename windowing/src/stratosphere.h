
#ifndef __STRATOSPHERE_H__
#define __STRATOSPHERE_H__

#include "testbed2.h"

void stratosphere_add(RTestBed2 testbed, size_t limit);
void stratosphere_apply(MANY(RWindowing) windowings, TCPC(MANY(PSet)) psets);

#endif