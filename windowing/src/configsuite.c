#include "configsuite.h"

#include "gatherer2.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

int parametersdefinition_init_done = 0;

ParameterDefinition parameters_definition[N_PARAMETERS];

void _configsuite_free(void*);
void _configsuite_io(IOReadWrite, FILE*, void**);
void _configsuite_hash(void* item, SHA256_CTX*);

G2Config g2_config_configsuite = {
    .element_size = sizeof(ConfigSuite),
    .freefn = _configsuite_free,
    .iofn = _configsuite_io,
    .hashfn = _configsuite_hash,
    .size = 0,
    .id = G2_CONFIGSUITE
};

char WINDOWING_NAMES[3][10] = {
    "QUERY",
    "RESPONSE",
    "BOTH"
};

char NN_NAMES[11][10] = {
    "NONE", // dns2
    "TLD", // dns2
    "ICANN", // dns2
    "PRIVATE", // dns2
    "",
    "",
    "",
    "ICANN", // dns
    "NONE", // dns
    "PRIVATE", // dns
    "TLD" // dns
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

void parameters_int_print(ParameterValue ref, int width, char str[20]) {
    int value;

    memcpy(&value, &ref.value, sizeof(int));
    
    sprintf(str, "%*d", width, value);
}

void parameters_double_print(ParameterValue ref, int width, char str[20]) {
    double value;

    memcpy(&value, &ref.value, sizeof(double));
    
    sprintf(str, "%*.3f", width, value);
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
    if(parametersdefinition_init_done) {
        return;
    }

    ParameterDefinition parameters_notconst[N_PARAMETERS];

    strcpy(parameters_notconst[PE_UNIQUE].format, "d");
    strcpy(parameters_notconst[PE_UNIQUE].name, "unique");
    parameters_notconst[PE_UNIQUE].size = sizeof(unique_t);
    parameters_notconst[PE_UNIQUE].print = parameters_int_print;

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

    strcpy(parameters_notconst[PE_NX_LOGIT_INCREMENT].format, "f");
    strcpy(parameters_notconst[PE_NX_LOGIT_INCREMENT].name, "nx_eps");
    parameters_notconst[PE_NX_LOGIT_INCREMENT].size = sizeof(nx_logit_increment_t);
    parameters_notconst[PE_NX_LOGIT_INCREMENT].print = parameters_double_print;

    memcpy((void*) parameters_definition, parameters_notconst, sizeof(ParameterDefinition) * N_PARAMETERS);

    parametersdefinition_init_done = 1;
}

void _configsuite_fill_pr(ConfigSuite* cs, ParameterGenerator pg) {

    #define __CPY(NAME_LW, NAME_UP) \
        MANY_INIT(cs->realm[PE_ ## NAME_UP], pg.NAME_LW ## _n, ParameterValue);\
        for (size_t i = 0; i < pg.NAME_LW ## _n; i++) {\
            cs->realm[PE_ ## NAME_UP]._[i].index = i;\
            memcpy(cs->realm[PE_ ## NAME_UP]._[i].value, &pg.NAME_LW[i], sizeof(NAME_LW ## _t));\
        }

    __CPY(unique, UNIQUE);
    __CPY(ninf, NINF);
    __CPY(pinf, PINF);
    __CPY(nn, NN);
    __CPY(wl_rank, WL_RANK);
    __CPY(wl_value, WL_VALUE);
    __CPY(windowing, WINDOWING);
    __CPY(nx_logit_increment, NX_LOGIT_INCREMENT);

    #undef __CPY
}

void _configsuite_fill_configs(ConfigSuite* cs) {
    size_t n_config = 1;

    for (size_t i = 0; i < N_PARAMETERS; i++) {
        n_config *= cs->realm[i].number;
    }

    MANY_INIT(cs->configs, n_config, Config);

    size_t idxconfig = 0;
    size_t idxs[N_PARAMETERS];
    memset(idxs, 0, sizeof(size_t) * N_PARAMETERS);
    #define PE_FOR(NAME) for (idxs[PE_ ## NAME] = 0; idxs[PE_ ## NAME] < cs->realm[PE_ ## NAME].number; ++idxs[PE_ ## NAME])
    PE_FOR(UNIQUE) {
        PE_FOR(NINF) {
            PE_FOR(PINF) {
                PE_FOR(NN) {
                    PE_FOR(WL_RANK) {
                        PE_FOR(WL_VALUE) {
                            PE_FOR(WINDOWING) {
                                PE_FOR(NX_LOGIT_INCREMENT) {

                                    Config* config = &cs->configs._[idxconfig];

                                    config->index = idxconfig;

                                    idxconfig++;

                                    void* ptr[N_PARAMETERS] = {
                                        &config->unique,
                                        &config->ninf,
                                        &config->pinf,
                                        &config->nn,
                                        &config->wl_rank,
                                        &config->wl_value,
                                        &config->windowing,
                                        &config->nx_logit_increment
                                    };

                                    // if (p >= 6) break;

                                    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                                        config->parameters[pp] = &cs->realm[pp]._[idxs[pp]];
                                        memcpy(ptr[pp], &cs->realm[pp]._[idxs[pp]].value, parameters_definition[pp].size);
                                    }
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
    count *= pg.nx_logit_increment_n;

    return count;
}

void _configsuite_build(ConfigSuite* cs, ParameterGenerator pg) {

    _parametersdefinition_init();

    _configsuite_fill_pr(cs, pg);
    _configsuite_fill_configs(cs);

    cs->generator = pg;
    if (pg.max_size && pg.max_size < cs->configs.number) {
        cs->configs.number = pg.max_size;
    }

    memcpy(&configsuite, cs, sizeof(ConfigSuite));
}

void configsuite_generate(ParameterGenerator pg) {
    if (g2_size(G2_CONFIGSUITE) > 0)
        LOG_ERROR("configsuite already initialized.");
    
    assert(g2_size(G2_CONFIGSUITE) == 0);
    
    ConfigSuite* cs = (ConfigSuite*) g2_insert_alloc_item(G2_CONFIGSUITE);

    _configsuite_build(cs, pg);
}

void configset_disable(ConfigSuite* cs) {
    for (size_t c = 0; c < cs->configs.number; c++) {
        int disabled = 0;
        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            disabled = cs->realm[pp]._[cs->configs._[c].parameters[pp]->index].disabled;
            if (disabled) break;
        }
        if (!disabled) {
            LOG_TRACE("Config %ld enabled", c);
        }
        cs->configs._[c].disabled = disabled;
    }
}

void _configsuite_free(void* item) {
    ConfigSuite* cs = (ConfigSuite*) item;

    MANY_FREE(cs->configs);
    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
        MANY_FREE(cs->realm[pp]);
    }
    // memset(&configsuite, 0, sizeof(ConfigSuite));
}

void _configsuite_io(IOReadWrite rw, FILE* file, void** item) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;
    ConfigSuite** cs = (ConfigSuite**) item;

    FRW((*cs)->generator);

    if (IO_IS_READ(rw)) {
        _configsuite_build((*cs), (*cs)->generator);
    }
}

void _configsuite_hash(void* item, SHA256_CTX* sha) {
    ConfigSuite* cs = (ConfigSuite*) item;

    for (size_t i = 0; i < cs->configs.number; i++) {
        Config* config = &cs->configs._[i];
        
        G2_IO_HASH_UPDATE(config->index);
        G2_IO_HASH_UPDATE(config->disabled);
        G2_IO_HASH_UPDATE(config->nn);
        G2_IO_HASH_UPDATE(config->wl_rank);
        G2_IO_HASH_UPDATE(config->windowing);

        G2_IO_HASH_UPDATE_DOUBLE(config->ninf);
        G2_IO_HASH_UPDATE_DOUBLE(config->pinf);
        G2_IO_HASH_UPDATE_DOUBLE(config->wl_value);
        G2_IO_HASH_UPDATE_DOUBLE(config->nx_logit_increment);
    }
}

void configsuite_print(const size_t idxconfig) {
    Config * config = &configsuite.configs._[idxconfig];

    printf("inf(%g,%g) %s (%ld,%g) %s %g",
        config->ninf, config->pinf,
        NN_NAMES[config->nn],
        config->wl_rank, config->wl_value,
        WINDOWING_NAMES[config->nn],
        config->nx_logit_increment
    );
}