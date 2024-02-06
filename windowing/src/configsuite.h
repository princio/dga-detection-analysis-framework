
#ifndef __WINDOWING_CONFIGSUITE_H__
#define __WINDOWING_CONFIGSUITE_H__

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



typedef struct ParameterGenerator {
    size_t max_size;
    
    size_t ninf_n;
    ninf_t ninf[100];

    size_t pinf_n;
    pinf_t pinf[100];

    size_t nn_n;
    nn_t nn[100];

    size_t wl_rank_n;
    wl_rank_t wl_rank[100];

    size_t wl_value_n;
    wl_value_t wl_value[100];

    size_t windowing_n;
    windowing_t windowing[100];

    size_t nx_epsilon_increment_n;
    nx_epsilon_increment_t nx_epsilon_increment[100];
} ParameterGenerator;




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

MAKEMANY(Config);

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

extern char WINDOWING_NAMES[3][10];
extern char NN_NAMES[11][10];
extern ParameterDefinition parameters_definition[N_PARAMETERS];

typedef struct ConfigSuite {
    ParameterGenerator pg;
    ParameterRealm pr;
    MANY(Config) configs;
} ConfigSuite;

size_t configsuite_pg_count(ParameterGenerator);

void configsuite_generate(ConfigSuite* cs, ParameterGenerator);
void configset_disable(ConfigSuite* cs);
void configset_free(ConfigSuite* cs);

#endif