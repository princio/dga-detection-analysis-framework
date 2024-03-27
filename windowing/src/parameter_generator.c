
#include "parameter_generator.h"

ParameterGenerator parametergenerator_default(size_t max_size) {
    ParameterGenerator pg;

    memset(&pg, 0, sizeof(ParameterGenerator));

    pg.max_size = max_size;

    #define __SET(NAME_LW) \
        pg.NAME_LW ## _n = sizeof(NAME_LW) / sizeof(NAME_LW ## _t);\
        for(size_t i = 0; i < pg.NAME_LW ## _n; i++) { pg.NAME_LW[i] = NAME_LW[i]; }

    {
        ninf_t ninf[] = {
            0,
            -10,
            -50,
            -150
        };
        __SET(ninf);
    }

    {
        pinf_t pinf[] = {
            0,
            10,
            50,
            150
        };
        __SET(pinf);
    }

    {
        nn_t nn[] = {
            // NN_NONE
            NN_TLD,
            // NN_ICANN,
            // NN_PRIVATE
        };
        __SET(nn);
    }

    {
        wl_rank_t wl_rank[] = {
            // 0,
            // 100,
            1000,
            100000
        };
        __SET(wl_rank);
    }

    {
        wl_value_t wl_value[] = {
            // 0,
            // -10,
            -50,
            -100,
            -150
        };
        __SET(wl_value);
    }

    {
        windowing_t windowing[] = {
            WINDOWING_Q,
            // WINDOWING_R,
            // WINDOWING_QR
        };
        __SET(windowing);
    }

    {
        nx_epsilon_increment_t nx_epsilon_increment[] = {
            0,
            0.05,
            0.1,
        };
        __SET(nx_epsilon_increment);
    }

    #undef __SET

    return pg;
}

void make_parameters_toignore(ConfigSuite* cs) {
    {
        size_t idx = 0;
        cs->realm[PE_NINF]._[idx++].disabled = 0; // 0
        cs->realm[PE_NINF]._[idx++].disabled = 0; // -10
        cs->realm[PE_NINF]._[idx++].disabled = 0; // -25
        cs->realm[PE_NINF]._[idx++].disabled = 0; // -50
        cs->realm[PE_NINF]._[idx++].disabled = 0; // -100
        cs->realm[PE_NINF]._[idx++].disabled = 0; // -150
    }
    {
        size_t idx = 0;
        cs->realm[PE_PINF]._[idx++].disabled = 0; // 0
        cs->realm[PE_PINF]._[idx++].disabled = 0; // 10
        cs->realm[PE_PINF]._[idx++].disabled = 0; // 25
        cs->realm[PE_PINF]._[idx++].disabled = 0; // 50
        cs->realm[PE_PINF]._[idx++].disabled = 0; // 100
        cs->realm[PE_PINF]._[idx++].disabled = 0; // 150
    }
    {
        size_t idx = 0;
        cs->realm[PE_NN]._[idx++].disabled = 0; // NN_NONE
        cs->realm[PE_NN]._[idx++].disabled = 0; // NN_TLD
        cs->realm[PE_NN]._[idx++].disabled = 0; // NN_ICANN
        cs->realm[PE_NN]._[idx++].disabled = 0; // NN_PRIVATE
    }
    {
        size_t idx = 0;
        cs->realm[PE_WL_RANK]._[idx++].disabled = 0; // 0
        cs->realm[PE_WL_RANK]._[idx++].disabled = 0; // 100
        cs->realm[PE_WL_RANK]._[idx++].disabled = 0; // 1000
        cs->realm[PE_WL_RANK]._[idx++].disabled = 0; // 10000
        cs->realm[PE_WL_RANK]._[idx++].disabled = 0; // 100000
    }
    {
        size_t idx = 0;
        cs->realm[PE_WL_VALUE]._[idx++].disabled = 0; // 0
        cs->realm[PE_WL_VALUE]._[idx++].disabled = 0; // -10
        cs->realm[PE_WL_VALUE]._[idx++].disabled = 0; // -25
        cs->realm[PE_WL_VALUE]._[idx++].disabled = 0; // -50
        cs->realm[PE_WL_VALUE]._[idx++].disabled = 0; // -100
        cs->realm[PE_WL_VALUE]._[idx++].disabled = 0; // -150
    }
    {
        size_t idx = 0;
        cs->realm[PE_WINDOWING]._[idx++].disabled = 0; // WINDOWING_Q
        cs->realm[PE_WINDOWING]._[idx++].disabled = 0; // WINDOWING_R
        cs->realm[PE_WINDOWING]._[idx++].disabled = 0; // WINDOWING_QR
    }
    {
        size_t idx = 0;
        cs->realm[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0
        cs->realm[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.05
        cs->realm[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.1
        cs->realm[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.25
        cs->realm[PE_NX_EPSILON_INCREMENT]._[idx++].disabled = 0; // 0.5
    }
}