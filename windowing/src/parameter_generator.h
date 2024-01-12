
#ifndef WINDOWING_PG_H
#define WINDOWING_PG_H

#include "configsuite.h"

ParameterGenerator parametergenerator_default(size_t max_size);

void make_parameters_toignore(ConfigSuite* cs);

#endif