#include "stat.h"

#include "dataset.h"
#include "io.h"
#include "logger.h"
#include "parameters.h"
#include "detect.h"
#include "sources.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef size_t index_t;
typedef double ninf_t;
typedef double pinf_t;
typedef NN nn_t;
typedef size_t wl_rank_t;
typedef double wl_value_t;
typedef WindowingType windowing_t;
typedef float nx_epsilon_increment_t;

MAKEMANY(index_t);
MAKEMANY(ninf_t);
MAKEMANY(pinf_t);
MAKEMANY(nn_t);
MAKEMANY(wl_rank_t);
MAKEMANY(wl_value_t);
MAKEMANY(windowing_t);
MAKEMANY(nx_epsilon_increment_t);

typedef struct StatPSet {
    MANY(ninf_t) ninf;
    MANY(pinf_t) pinf;
    MANY(nn_t) nn;
    MANY(wl_rank_t) wl_rank;
    MANY(wl_value_t) wl_value;
    MANY(windowing_t) windowing;
    MANY(nx_epsilon_increment_t) nx_epsilon_increment;
} StatPSet;

#define FMT_ninf "f"
#define FMT_pinf "f"
#define FMT_nn "s"
#define FMT_wl_rank "ld"
#define FMT_wl_value "f"
#define FMT_windowing "s"
#define FMT_nx_epsilon_increment "f"

#define FMT_PRE_ninf(V) V
#define FMT_PRE_pinf(V) V
#define FMT_PRE_nn(V) NN_NAMES[V]
#define FMT_PRE_wl_rank(V) V
#define FMT_PRE_wl_value(V) V
#define FMT_PRE_windowing(V) WINDOWING_NAMES[V]
#define FMT_PRE_nx_epsilon_increment(V) V

#define multi_psetitem(FUN)\
FUN(ninf);\
FUN(pinf);\
FUN(nn);\
FUN(wl_rank);\
FUN(wl_value);\
FUN(windowing);\
FUN(nx_epsilon_increment);

StatPSet _parameters_count(MANY(TestBed2Apply) applies) {
    StatPSet sp;
    
    #define pc_init(NAME)\
    INITMANY(sp.NAME, applies.number, NAME ## _t);\
    sp.NAME.number = 0;

    #define pc_search(NAME) {\
            int found = 0;\
            for (size_t idx_ ## NAME = 0; idx_ ## NAME < sp.NAME.number; idx_ ## NAME++) {\
                if (applies._[idxpset].pset.NAME ==  sp.NAME._[idx_ ## NAME]) {\
                    found = 1;\
                }\
            }\
            if (!found) { sp.NAME._[sp.NAME.number++] = applies._[idxpset].pset.NAME; }\
        }

    multi_psetitem(pc_init);

    for (size_t idxpset = 0; idxpset < applies.number; idxpset++) {
        multi_psetitem(pc_search);
    }

    #undef pc_init
    #undef pc_search

    return sp;
}

Stat stat_run(RTrainer trainer) {

    TrainerResults* results = &trainer->results;

    Stat stats;

    StatPSet psetitems = _parameters_count(trainer->tb2->applies);

    #define init_statmetric(NAME) DGAFOR(cl) DGAFOR(cl) {\
        sm->NAME[cl].min = DBL_MAX;\
        sm->NAME[cl].max = - DBL_MAX;\
        sm->NAME[cl].avg = 0;\
        sm->NAME[cl].std = 0;\
        sm->NAME[cl].count = 0;\
    }

    #define init_stat(NAME) \
    {\
        INITMANY(stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser].by ## NAME, psetitems.NAME.number, StatByPSetItemValue);\
        for (size_t __idx = 0; __idx < psetitems.NAME.number; __idx++) {\
            StatByPSetItemValue* sm = &stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser].by ## NAME._[__idx];\
            strcpy(sm->name, #NAME);\
            sprintf(sm->svalue, "%"FMT_ ## NAME, FMT_PRE_ ## NAME(psetitems.NAME._[__idx]));\
            init_statmetric(tr);\
        }\
    }

    #define update_statmetric(NAME, statmetric_value) \
    {\
        if (sm->NAME[cl].min > statmetric_value) sm->NAME[cl].min = statmetric_value;\
        if (sm->NAME[cl].max < statmetric_value) sm->NAME[cl].max = statmetric_value;\
        sm->NAME[cl].avg += statmetric_value;\
        sm->NAME[cl].count++;\
    }

    #define update_stat(NAME) \
    {\
        StatByPSetItemValue* sm;\
        size_t idxpsetitem;\
        for (idxpsetitem = 0; idxpsetitem < psetitems.NAME.number; idxpsetitem++) {\
            int cmp = memcmp(&trainer->tb2->applies._[idxapply].pset.NAME, &psetitems.NAME._[idxpsetitem], sizeof(NAME ## _t));\
            if (cmp == 0) {\
                break;\
            }\
        }\
        sm = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, NAME, idxpsetitem);\
        DGAFOR(cl) {\
            const double value_tr = DETECT_TRUERATIO(result->best_test, cl);\
            update_statmetric(tr, value_tr);\
        }\
    }

    #define finalize_stat(NAME) \
    {\
        for (size_t idxpsetitem = 0; idxpsetitem < psetitems.NAME.number; idxpsetitem++) {\
            StatByPSetItemValue* sm;\
            sm = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, NAME, idxpsetitem);\
            DGAFOR(cl) { sm->tr[cl].avg /= sm->tr[cl].count; }\
        }\
    }

    INITMANY(stats.by.byfold, trainer->tb2->folds.number, StatByFold);
    for (size_t idxfold = 0; idxfold < trainer->tb2->folds.number; idxfold++) {
        INITMANY(stats.by.byfold._[idxfold].bywsize, trainer->tb2->wsizes.number, StatByWSize);
        for (size_t idxwsize = 0; idxwsize < trainer->tb2->wsizes.number; idxwsize++) {
            INITMANY(stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser, trainer->thchoosers.number, StatByThChooser);
            for (size_t idxthchooser = 0; idxthchooser < trainer->thchoosers.number; idxthchooser++) {
                multi_psetitem(init_stat);
            }
        }
    }

    for (size_t idxwsize = 0; idxwsize < results->bywsize.number; idxwsize++) {
        for (size_t idxapply = 0; idxapply < results->bywsize._[idxwsize].byapply.number; idxapply++) {
            for (size_t idxfold = 0; idxfold < results->bywsize._[idxwsize].byapply._[idxapply].byfold.number; idxfold++) {
                for (size_t idxtry = 0; idxtry < results->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry.number; idxtry++) {
                    for (size_t idxsplit = 0; idxsplit < results->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry._[idxtry].bysplits.number; idxsplit++) {
                        for (size_t idxthchooser = 0; idxthchooser < results->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry._[idxtry].bysplits._[idxsplit].bythchooser.number; idxthchooser++) {
                            Result* result;
                            result = &RESULT_IDX((*results), idxwsize, idxapply, idxfold, idxtry, idxsplit, idxthchooser);
                            multi_psetitem(update_stat);
                        }
                    }
                }
            }
        }
    }

    for (size_t idxwsize = 0; idxwsize < results->bywsize.number; idxwsize++) {
        for (size_t idxapply = 0; idxapply < results->bywsize._[idxwsize].byapply.number; idxapply++) {
            for (size_t idxfold = 0; idxfold < results->bywsize._[idxwsize].byapply._[idxapply].byfold.number; idxfold++) {
                for (size_t idxtry = 0; idxtry < results->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry.number; idxtry++) {
                    for (size_t idxsplit = 0; idxsplit < results->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry._[idxtry].bysplits.number; idxsplit++) {
                        for (size_t idxthchooser = 0; idxthchooser < results->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry._[idxtry].bysplits._[idxsplit].bythchooser.number; idxthchooser++) {
                            multi_psetitem(finalize_stat)
                        }
                    }
                }
            }
        }
    }

    #define prin_stat(NAME)\
    for (size_t idxpsetitem = 0; idxpsetitem < psetitems.NAME.number; idxpsetitem++) {\
        StatByPSetItemValue* item = &stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser].by ## NAME._[idxpsetitem];\
        printf("%-8ld ", idxfold);\
        printf("%-8ld ", trainer->tb2->wsizes._[idxwsize].value);\
        printf("%-15s ", trainer->thchoosers._[idxthchooser].name);\
        printf("%-25s ", item->name);\
        printf("%-20s ", item->svalue);\
        DGAFOR(cl) {\
            printf("%-5.2f ", item->tr[cl].min);\
            printf("%-5.2f ", item->tr[cl].max);\
            printf("%-5.2f      ", item->tr[cl].avg);\
        }\
        printf("\n");\
    }

    for (size_t idxfold = 0; idxfold < trainer->tb2->folds.number; idxfold++) {
        for (size_t idxwsize = 0; idxwsize < trainer->tb2->wsizes.number; idxwsize++) {
            for (size_t idxthchooser = 0; idxthchooser < trainer->thchoosers.number; idxthchooser++) {
                multi_psetitem(prin_stat);
            }
        }
    }

    #define pc_free(NAME) FREEMANY(psetitems.NAME);

    multi_psetitem(pc_free);

    return stats;
}

void stat_free(Stat stat) {

    #define free_psetitem(NAME)\
        FREEMANY(stat.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser].by ## NAME);


    for (size_t idxfold = 0; idxfold < stat.by.byfold.number; idxfold++) {
        for (size_t idxwsize = 0; idxwsize < stat.by.byfold._[idxfold].bywsize.number; idxwsize++) {
            for (size_t idxthchooser = 0; idxthchooser < stat.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser.number; idxthchooser++) {
                multi_psetitem(free_psetitem);
            }
            FREEMANY(stat.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser);
        }
        FREEMANY(stat.by.byfold._[idxfold].bywsize);
    }
    FREEMANY(stat.by.byfold);
}