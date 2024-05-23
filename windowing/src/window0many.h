
#ifndef __WINDOWING_WINDOW0MANY_H__
#define __WINDOWING_WINDOW0MANY_H__

#include "common.h"

#include "gatherer2.h"
#include "wapply.h"

#define NOT !
#define ISZERO 0 ==

typedef struct __Window {
    G2Index manyindex;

    size_t index;
    
    RWindowing windowing;

    size_t n_message;

    uint32_t fn_req_min;
    uint32_t fn_req_max;

    double time_s_start;
    double time_s_end;

    size_t qr;
    size_t u; //unique
    size_t q;
    size_t r;
    size_t nx;

    size_t whitelisted[N_WHITELISTINGRANK];

    size_t eps[N_NN][N_DETZONE];
    
    size_t ninf[N_NN];
    size_t pinf[N_NN];
    double llr[N_NN][N_WHITELISTINGRANK];

    MANY(WApply) applies;
} __Window;

typedef struct __Window0Many {
    G2Index g2index;

    RWindowing windowing;
    
    size_t element_size;
    size_t size;
    size_t number;
    __Window* _;
} __Window0Many;

void window0many_buildby_size(RWindow0Many, const size_t);

RWindowMany window0many_create(size_t);

void window0many_updatewindow(RWindow window, DNSMessageGrouped* message);

#endif