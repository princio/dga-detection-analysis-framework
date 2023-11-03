#ifndef __CALCULATOR_H
#define __CALCULATOR_H

#include "parameters.h"
#include "windowing.h"

void calculator_1message_Npsets(const DNSMessage message, const MANY(PSet) psets, MANY(WindowingWindows)* sw);

#endif