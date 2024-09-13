#ifndef __WAPPLY_H__
#define __WAPPLY_H__

#include "common.h"

#include "configsuite.h"

#define WAPPLY_GET(APPLY, WSIZE_INDEX, PSET_INDEX)

typedef struct WApplyTiny {
    uint16_t wcount;
    double  logit;
    uint16_t whitelistened;
} WApplyTiny;

typedef struct WApply {
    uint16_t wcount;
    double  logit;
    uint16_t whitelistened_unique;
    uint16_t whitelistened_total;
    uint16_t dn_bad[N_DETZONE];
} WApply;

MAKEMANY(WApplyTiny);
MAKEMANY(WApply);

typedef struct WApplyArgs
{
    int id;
    WApply* wapply;
    TCPC(DNSMessage) message;
    Config* config;
} WApplyArgs;

void wapply_grouped_run(WApply* wapply, DNSMessageGrouped* message, Config* config);

void wapply_run(WApply* wapply, DNSMessage* message, Config* config);

#endif