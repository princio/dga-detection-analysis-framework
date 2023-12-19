#ifndef __WAPPLY_H__
#define __WAPPLY_H__

#include "common.h"

#include "parameters.h"

#define WAPPLY_GET(APPLY, WSIZE_INDEX, PSET_INDEX)

typedef struct WApply {
    uint32_t pset_index;

    uint16_t wcount;
    double  logit;
    uint16_t whitelistened;
    uint16_t dn_bad_05;
    uint16_t dn_bad_09;
    uint16_t dn_bad_099;
    uint16_t dn_bad_0999;
} WApply;

MAKEMANY(WApply);

void wapply_init(RWindow, const size_t psets_number);
void wapply_init_many(MANY(RWindow), MANY(WSize), MANY(PSet));
void wapply_run(WApply* wapply, TCPC(DNSMessage) message, TCPC(PSet) pset);
void wapply_run_many(MANY(WApply)* applies, TCPC(DNSMessage) message, TCPC(MANY(PSet)) psets);

#endif