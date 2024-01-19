#include "configsuite.h"

// #include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

int parametersdefinition_init_done = 0;

ParameterDefinition parameters_definition[N_PARAMETERS];

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

void _configsuite_fill_pr(ConfigSuite* cs, ParameterGenerator pg) {

    #define __CPY(NAME_LW, NAME_UP) \
        MANY_INIT(cs->pr[PE_ ## NAME_UP], pg.NAME_LW ## _n, ParameterValue);\
        for (size_t i = 0; i < pg.NAME_LW ## _n; i++) {\
            cs->pr[PE_ ## NAME_UP]._[i].index = i;\
            memcpy(cs->pr[PE_ ## NAME_UP]._[i].value, &pg.NAME_LW[i], sizeof(NAME_LW ## _t));\
        }

    __CPY(ninf, NINF);
    __CPY(pinf, PINF);
    __CPY(nn, NN);
    __CPY(wl_rank, WL_RANK);
    __CPY(wl_value, WL_VALUE);
    __CPY(windowing, WINDOWING);
    __CPY(nx_epsilon_increment, NX_EPSILON_INCREMENT);

    #undef __CPY
}

void _configsuite_fill_configs(ConfigSuite* cs) {
    size_t n_config = 1;

    for (size_t i = 0; i < N_PARAMETERS; i++) {
        n_config *= cs->pr[i].number;
    }

    MANY_INIT(cs->configs, n_config, Config);

    size_t idxconfig = 0;
    size_t idxs[N_PARAMETERS];
    memset(idxs, 0, sizeof(size_t) * N_PARAMETERS);
    #define PE_FOR(NAME) for (idxs[PE_ ## NAME] = 0; idxs[PE_ ## NAME] < cs->pr[PE_ ## NAME].number; ++idxs[PE_ ## NAME])
    PE_FOR(NINF) {
        PE_FOR(PINF) {
            PE_FOR(NN) {
                PE_FOR(WL_RANK) {
                    PE_FOR(WL_VALUE) {
                        PE_FOR(WINDOWING) {
                            PE_FOR(NX_EPSILON_INCREMENT) {

                                Config* config = &cs->configs._[idxconfig];

                                config->index = idxconfig;

                                idxconfig++;

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
                                    config->parameters[pp] = &cs->pr[pp]._[idxs[pp]];
                                    memcpy(ptr[pp], &cs->pr[pp]._[idxs[pp]].value, parameters_definition[pp].size);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    #undef PE_FOR
}

size_t configsuite_pg_count(ParameterGenerator pg) {
    size_t count = 1;

    count *= pg.ninf_n;
    count *= pg.pinf_n;
    count *= pg.nn_n;
    count *= pg.wl_rank_n;
    count *= pg.wl_value_n;
    count *= pg.windowing_n;
    count *= pg.nx_epsilon_increment_n;

    return count;
}

void configsuite_generate(ConfigSuite* cs, ParameterGenerator pg) {
    if (0 == parametersdefinition_init_done) {
        _parametersdefinition_init();
        parametersdefinition_init_done = 1;
    }
    _configsuite_fill_pr(cs, pg);
    _configsuite_fill_configs(cs);
    cs->pg = pg;
    if (pg.max_size && pg.max_size < cs->configs.number) {
        cs->configs.number = pg.max_size;
    }
}

void configset_disable(ConfigSuite* cs) {
    for (size_t c = 0; c < cs->configs.number; c++) {
        int disabled = 0;
        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            disabled = cs->pr[pp]._[cs->configs._[c].parameters[pp]->index].disabled;
            if (disabled) break;
        }
        if (!disabled) {
            LOG_TRACE("Config %ld enabled", c);
        }
        cs->configs._[c].disabled = disabled;
    }
}

void configset_free(ConfigSuite* cs) {
    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
        MANY_FREE(cs->pr[pp]);
    }
    MANY_FREE(cs->configs);
}
