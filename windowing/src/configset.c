#include "configset.h"

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

const ParameterDefinition parameters_definition[N_PARAMETERS];
const ParameterRealm parameterrealm;
const ConfigSuite configsuite;

char WINDOWING_NAMES[3][10] = {
    "QUERY",
    "RESPONSE",
    "BOTH"
};

char NN_NAMES[11][10] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "ICANN",
    "NONE",
    "PRIVATE",
    "TLD"
};

const char parameters_format[N_PARAMETERS][5] = {
    "%f",
    "%f",
    "%s",
    "%ld",
    "%f",
    "%s",
    "%f"
};

void parameters_double_print(ParameterValue ref, int width, char str[20]) {
    double value;

    memcpy(&value, &ref.value, sizeof(double));
    
    sprintf(str, "%*f", width, value);
}

void parameters_size_t_print(ParameterValue ref, int width, char str[20]) {
    size_t value;
    
    memcpy(&value, &ref.value, sizeof(size_t));

    sprintf(str, "%*ld", width, value);
}

void parameters_nn_print(ParameterValue ref, int width, char str[20]) {
    nn_t value;
    
    memcpy(&value, &ref.value, sizeof(nn_t));

    sprintf(str, "%*s", width, NN_NAMES[value]);
}

void parameters_windowing_print(ParameterValue ref, int width, char str[20]) {
    windowing_t value;

    memcpy(&value, &ref.value, sizeof(windowing_t));

    sprintf(str, "%*s", width, WINDOWING_NAMES[value]);
}

void _parametersdefinition_init() {
    ParameterDefinition parameters_notconst[N_PARAMETERS];

    strcpy(parameters_notconst[PE_NINF].format, "f");
    strcpy(parameters_notconst[PE_NINF].name, "ninf");
    parameters_notconst[PE_NINF].size = sizeof(ninf_t);
    parameters_notconst[PE_NINF].print = parameters_double_print;

    strcpy(parameters_notconst[PE_PINF].format, "f");
    strcpy(parameters_notconst[PE_PINF].name, "pinf");
    parameters_notconst[PE_PINF].size = sizeof(pinf_t);
    parameters_notconst[PE_PINF].print = parameters_double_print;

    strcpy(parameters_notconst[PE_NN].format, "s");
    strcpy(parameters_notconst[PE_NN].name, "nn");
    parameters_notconst[PE_NN].size = sizeof(nn_t);
    parameters_notconst[PE_NN].print = parameters_nn_print;

    strcpy(parameters_notconst[PE_WL_RANK].format, "ld");
    strcpy(parameters_notconst[PE_WL_RANK].name, "wl_rank");
    parameters_notconst[PE_WL_RANK].size = sizeof(wl_rank_t);
    parameters_notconst[PE_WL_RANK].print = parameters_size_t_print;

    strcpy(parameters_notconst[PE_WL_VALUE].format, "f");
    strcpy(parameters_notconst[PE_WL_VALUE].name, "wl_value");
    parameters_notconst[PE_WL_VALUE].size = sizeof(wl_value_t);
    parameters_notconst[PE_WL_VALUE].print = parameters_double_print;

    strcpy(parameters_notconst[PE_WINDOWING].format, "s");
    strcpy(parameters_notconst[PE_WINDOWING].name, "windowing");
    parameters_notconst[PE_WINDOWING].size = sizeof(windowing_t);
    parameters_notconst[PE_WINDOWING].print = parameters_windowing_print;

    strcpy(parameters_notconst[PE_NX_EPSILON_INCREMENT].format, "f");
    strcpy(parameters_notconst[PE_NX_EPSILON_INCREMENT].name, "nx_epsilon_increment");
    parameters_notconst[PE_NX_EPSILON_INCREMENT].size = sizeof(nx_epsilon_increment_t);
    parameters_notconst[PE_NX_EPSILON_INCREMENT].print = parameters_double_print;

    memcpy((void*) parameters_definition, parameters_notconst, sizeof(ParameterDefinition) * N_PARAMETERS);
}

void _parameterset_init() {
    ParameterRealm parameterset_notconst;
    memset(parameterset_notconst, 0, sizeof(ParameterRealm));

    #define __CPY(NAME_LW, NAME_UP) \
        INITMANY(parameterset_notconst[PE_ ## NAME_UP], sizeof(NAME_LW) / sizeof(NAME_LW ## _t), ParameterValue);\
        for (size_t i = 0; i < sizeof(NAME_LW) / sizeof(NAME_LW ## _t); i++) {\
            parameterset_notconst[PE_ ## NAME_UP]._[i].index = i;\
            memcpy(parameterset_notconst[PE_ ## NAME_UP]._[i].value, &NAME_LW[i], sizeof(NAME_LW ## _t));\
        }

    {
        ninf_t ninf[] = {
            0,
            -10,
            -25,
            -50,
            -100,
            -150
        };
        __CPY(ninf, NINF);
    }

    {
        pinf_t pinf[] = {
            0,
            10,
            25,
            50,
            100,
            150
        };
        __CPY(pinf, PINF);
    }

    {
        nn_t nn[] = {
            NN_NONE,
            NN_TLD,
            NN_ICANN,
            NN_PRIVATE
        };
        __CPY(nn, NN);
    }

    {
        wl_rank_t wl_rank[] = {
            0,
            100,
            1000,
            10000,
            100000
        };
        __CPY(wl_rank, WL_RANK);
    }

    {
        wl_value_t wl_value[] = {
            0,
            -10,
            -25,
            -50,
            -100,
            -150
        };
        __CPY(wl_value, WL_VALUE);
    }

    {
        windowing_t windowing[] = {
            WINDOWING_Q,
            WINDOWING_R,
            WINDOWING_QR
        };
        __CPY(windowing, WINDOWING);
    }

    {
        nx_epsilon_increment_t nx_epsilon_increment[] = {
            0,
            0.05,
            0.1,
            0.25,
            0.5
        };
        __CPY(nx_epsilon_increment, NX_EPSILON_INCREMENT);
    }

    #undef __CPY

    memcpy((void*) parameterrealm, parameterset_notconst, sizeof(ParameterRealm));
}

void _configsuite_init() {
    ConfigSuite configsuite_notconst;

    size_t n_psets = 1;

    for (size_t i = 0; i < N_PARAMETERS; i++) {
        n_psets *= parameterrealm[i].number;
    }

    INITMANY(configsuite_notconst, n_psets, Config);

    size_t p = 0;
    size_t idxs[N_PARAMETERS];
    memset(idxs, 0, sizeof(size_t) * N_PARAMETERS);
    #define PE_FOR(NAME) for (idxs[PE_ ## NAME] = 0; idxs[PE_ ## NAME] < parameterrealm[PE_ ## NAME].number; ++idxs[PE_ ## NAME])
    PE_FOR(NINF) {
        PE_FOR(PINF) {
            PE_FOR(NN) {
                PE_FOR(WL_RANK) {
                    PE_FOR(WL_VALUE) {
                        PE_FOR(WINDOWING) {
                            PE_FOR(NX_EPSILON_INCREMENT) {

                                Config* config = &configsuite_notconst._[p];

                                config->index = p;

                                p++;

                                void* ptr[N_PARAMETERS] = {
                                    &config->ninf,
                                    &config->pinf,
                                    &config->nn,
                                    &config->wl_rank,
                                    &config->wl_value,
                                    &config->windowing,
                                    &config->nx_epsilon_increment
                                };

                                // if (p >= 6) break;

                                for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                                    config->parameters[pp] = &parameterrealm[pp]._[idxs[pp]];
                                    memcpy(ptr[pp], &parameterrealm[pp]._[idxs[pp]].value, parameters_definition[pp].size);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    #undef PE_FOR
    memcpy((void*) &configsuite, &configsuite_notconst, sizeof(ConfigSuite));
}

void configset_init() {
    _parametersdefinition_init();
    _parameterset_init();
    _configsuite_init();
}

void configset_disable() {
    for (size_t c = 0; c < configsuite.number; c++) {
        int disabled = 0;
        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            disabled = parameterrealm[pp]._[configsuite._[c].parameters[pp]->index].disabled;
            if (disabled) break;
        }
        if (!disabled) LOG_DEBUG("Config %ld enabled", c);
        configsuite._[c].disabled = disabled;
    }
}

void configset_free() {
    ParameterRealm parameterset_notconst;
    ConfigSuite configsuite_notconst;
    memcpy(&configsuite_notconst, &configsuite, sizeof(ConfigSuite));
    memcpy(parameterset_notconst, parameterrealm, sizeof(ParameterRealm));
    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
        FREEMANY(parameterset_notconst[pp]);
    }
    FREEMANY(configsuite_notconst);
}
