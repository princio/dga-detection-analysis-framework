#ifndef __WINDOWINGTESTBED2_H__
#define __WINDOWINGTESTBED2_H__

#include "common.h"

#include "dataset.h"
#include "configset.h"
#include "wapply.h"
#include "windowing.h"

#include <linux/limits.h>

#define TB2_WSIZES_N(A) A->wsizes.number


/* Windowing */
/* Windowing */
/* Windowing */


typedef RWindowing TestBed2WindowingBy_source;
typedef MANY(RWindowing) MANY(TestBed2WindowingBy_source);

typedef struct TestBed2WindowingBy_wsize {
    MANY(TestBed2WindowingBy_source) bysource;
} TestBed2WindowingBy_wsize;
MAKEMANY(TestBed2WindowingBy_wsize);

typedef struct TestBed2WindowingBy {
    struct {
        const size_t wsize;
        const size_t source;
    } n;
    MANY(TestBed2WindowingBy_wsize) bywsize;
} TestBed2WindowingBy;





/* Dataset */
/* Dataset */
/* Dataset */

typedef DatasetSplits TestBed2DatasetBy_fold;
MAKEMANY(TestBed2DatasetBy_fold);

typedef struct TestBed2DatasetBy_try {
    RDataset dataset;
    MANY(TestBed2DatasetBy_fold) byfold;
} TestBed2DatasetBy_try;
MAKEMANY(TestBed2DatasetBy_try);

typedef struct TestBed2DatasetBy_wsize {
    MANY(TestBed2DatasetBy_try) bytry;
} TestBed2DatasetBy_wsize;
MAKEMANY(TestBed2DatasetBy_wsize);

typedef struct FoldConfig {
    size_t k;
    size_t k_test;
} FoldConfig;

MAKEMANY(FoldConfig);

typedef struct TestBed2DatasetBy {
    struct {
        const size_t wsize;
        const size_t try;
        const size_t fold;
    } n;

    MANY(FoldConfig) folds;
    MANY(TestBed2DatasetBy_wsize) bywsize;
} TestBed2DatasetBy;

/* Apply */
/* Apply */
/* Apply */

/* __TestBed2 */
/* __TestBed2 */
/* __TestBed2 */

typedef struct __TestBed2 {
    MANY(ConfigApplied) applies;

    MANY(WSize) wsizes;

    MANY(RSource) sources;

    TestBed2WindowingBy windowing;

    TestBed2DatasetBy dataset;
} __TestBed2;


RTestBed2 testbed2_create(MANY(WSize), const size_t);


void testbed2_set_configapplied(RTestBed2);
void testbed2_source_add(RTestBed2, RSource);
void testbed2_windowing(RTestBed2);
void testbed2_fold_add(RTestBed2, FoldConfig);
int testbed2_try_set(RTestBed2, size_t);
void testbed2_addpsets(RTestBed2);
void testbed2_apply(RTestBed2);
void testbed2_free(RTestBed2);

#endif