#ifndef _TB2W_H__
#define _TB2W_H__

#include "common.h"

#include "dataset.h"
#include "configsuite.h"
#include "wapply.h"
#include "windowing.h"

#include <linux/limits.h>

#define TB2_WSIZES_N(A) A->wsizes.number


typedef struct TB2WBy {
    struct {
        size_t source;
    } n;
    MANY(RWindowing) bysource;
} TB2WBy;


typedef struct TB2W {
    char rootdir[DIR_MAX];
    ConfigSuite configsuite;
    size_t wsize;
    MANY(RSource) sources;
    TB2WBy windowing;
} TB2W;

RTB2W tb2w_create(char rootdir[DIR_MAX], size_t wsize, const ParameterGenerator pg);

void tb2w_set_configapplied(RTB2W);
void tb2w_source_add(RTB2W, RSource);
void tb2w_windowing(RTB2W);
void tb2w_apply(RTB2W);
void tb2w_free(RTB2W);

#endif