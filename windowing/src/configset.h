
#ifndef __WINDOWING_CONFIGSET_H__
#define __WINDOWING_CONFIGSET_H__

#include "common.h"

#include "io.h"

#define N_PARAMETERS 7




typedef double ninf_t;
typedef double pinf_t;
typedef NN nn_t;
typedef size_t wl_rank_t;
typedef double wl_value_t;
typedef WindowingType windowing_t;
typedef double nx_epsilon_increment_t;

typedef enum ParametersEnum {
    PE_NINF,
    PE_PINF,
    PE_NN,
    PE_WL_RANK,
    PE_WL_VALUE,
    PE_WINDOWING,
    PE_NX_EPSILON_INCREMENT
} ParametersEnum;







typedef struct ParameterValue {
    size_t index;
    uint8_t value[16];
    int disabled;
} ParameterValue;

MAKEMANY(ParameterValue);

typedef MANY(ParameterValue) ParameterRealm[N_PARAMETERS];

typedef int ParameterValueEnabled;
MAKEMANY(ParameterValueEnabled);
typedef MANY(ParameterValueEnabled) ParameterRealmEnabled[N_PARAMETERS];








typedef struct Config {
    int disabled;
    
    size_t index;

    const ParameterValue* parameters[N_PARAMETERS];
    
    double ninf;
    double pinf;
    NN nn;
    size_t wl_rank;
    double wl_value;
    WindowingType windowing;
    double nx_epsilon_increment;
} Config;

MAKEMANYNAME(ConfigSuite, Config);






typedef void (ParameterPreFunc)(ParameterValue ref, int width, char str[20]);

typedef struct ParameterDefinition {
    size_t size;
    char format[5];
    char name[100];
    ParameterPreFunc* print;
} ParameterDefinition;



typedef struct ConfigApplied {
    int applied;
    size_t index;
    Config* config;
} ConfigApplied;

MAKEMANY(ConfigApplied);



char WINDOWING_NAMES[3][10];
char NN_NAMES[11][10];
const ParameterDefinition parameters_definition[N_PARAMETERS];
const ParameterRealm parameterrealm;
const ConfigSuite configsuite;
const char parameters_format[N_PARAMETERS][5];

void configset_init();
void configset_disable();
void configset_free();

#endif