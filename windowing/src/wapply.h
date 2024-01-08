#ifndef __WAPPLY_H__
#define __WAPPLY_H__

#include "common.h"

#include "configset.h"

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

void wapply_init(RWindow, const size_t);
void wapply_run_many(MANY(WApply)*, TCPC(DNSMessage), MANY(ConfigApplied));

#endif