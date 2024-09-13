
#ifndef WINDOWING_PG_H
#define WINDOWING_PG_H

#include "configsuite.h"

ParameterGenerator parametergenerator_default_tiny();
ParameterGenerator parametergenerator_default();

void make_parameters_toignore(ConfigSuite* cs);

#endif