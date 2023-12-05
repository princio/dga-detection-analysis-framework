#ifndef __WAPPLY_H__
#define __WAPPLY_H__

#include "common.h"

#include "parameters.h"

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

#define WAPPLY_GET(APPLY, WSIZE_INDEX, PSET_INDEX)

typedef struct WApply {
    size_t pset_index;

    int32_t wcount;
    double  logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} WApply;

MAKEMANY(WApply);

void wapply_init(RWindow0, const size_t psets_number);
void wapply_init_many(MANY(RWindow0), MANY(WSize), MANY(PSet));
void wapply_run(WApply* wapply, TCPC(DNSMessage) message, TCPC(PSet) pset);
void wapply_run_many(MANY(WApply)* applies, TCPC(DNSMessage) message, TCPC(MANY(PSet)) psets);

#endif