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

    TrainerBy* by = &trainer->by;

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
            init_statmetric(train);\
            init_statmetric(test);\
        }\
    }

    #define update_statmetric(NAME) \
    {\
        const double value = DETECT_TRUERATIO(result->best_ ## NAME, cl);\
        if (sm->NAME[cl].min > value) sm->NAME[cl].min = value;\
        if (sm->NAME[cl].max < value) sm->NAME[cl].max = value;\
        sm->NAME[cl].avg += value;\
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
            update_statmetric(train);\
            update_statmetric(test);\
        }\
    }

    #define finalize_statmetric(NAME) DGAFOR(cl) { sm->NAME[cl].avg /= sm->NAME[cl].count; }

    #define finalize_stat(NAME) \
    {\
        for (size_t idxpsetitem = 0; idxpsetitem < psetitems.NAME.number; idxpsetitem++) {\
            StatByPSetItemValue* sm;\
            sm = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, NAME, idxpsetitem);\
            finalize_statmetric(train);\
            finalize_statmetric(test);\
        }\
    }

    memcpy((size_t*) &stats.by.n.fold, &trainer->tb2->folds.number, sizeof(size_t));
    memcpy((size_t*) &stats.by.n.wsize, &trainer->tb2->wsizes.number, sizeof(size_t));
    memcpy((size_t*) &stats.by.n.thchooser, &trainer->thchoosers.number, sizeof(size_t));

    INITMANY(stats.by.byfold, stats.by.n.fold, StatByFold);
    FORBY(stats.by, fold) {
        INITMANY(stats.by.byfold._[idxfold].bywsize, stats.by.n.wsize, StatByWSize);
        FORBY(stats.by, wsize) {
            INITMANY(stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser, stats.by.n.thchooser, StatByThChooser);
            FORBY(stats.by, thchooser) {
                multi_psetitem(init_stat);
            }
        }
    }

    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                FORBY((*by), apply) {
                    for (size_t idxtry = 0; idxtry < by->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry.number; idxtry++) {
                        for (size_t idxsplit = 0; idxsplit < by->bywsize._[idxwsize].byapply._[idxapply].byfold._[idxfold].bytry._[idxtry].bysplits.number; idxsplit++) {
                            TrainerBy_thchooser* result;
                            result = &RESULT_IDX((*by), idxwsize, idxapply, idxfold, idxtry, idxsplit, idxthchooser);
                            multi_psetitem(update_stat);
                        }
                    }
                }
            }
        }
    }

    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                multi_psetitem(finalize_stat)
            }
        }
    }

    #define print_statmetric\
        DGAFOR(cl) {\
            printf("%7.2f~%-7.2f", item->train[cl].min, item->test[cl].min);\
            printf("%7.2f~%-7.2f", item->train[cl].max, item->test[cl].max);\
            printf("%7.2f~%-7.2f", item->train[cl].avg, item->test[cl].avg);\
            printf("    ");\
        }\

    #define print_statmetric_2d\
        DGAFOR(cl) {\
            printf("%5d,%-5d", (int) (item->train[cl].min * 100), (int) (item->test[cl].min * 100));\
            printf("%5d,%-5d", (int) (item->train[cl].max * 100), (int) (item->test[cl].max * 100));\
            printf("%5d,%-5d", (int) (item->train[cl].avg * 100), (int) (item->test[cl].avg * 100));\
            printf("    ");\
        }\

    #define prin_stat(NAME)\
    for (size_t idxpsetitem = 0; idxpsetitem < psetitems.NAME.number; idxpsetitem++) {\
        StatByPSetItemValue* item = &stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser].by ## NAME._[idxpsetitem];\
        printf("%-8ld ", idxfold);\
        printf("%-8ld ", trainer->tb2->wsizes._[idxwsize].value);\
        printf("%-15s ", trainer->thchoosers._[idxthchooser].name);\
        printf("%-25s ", item->name);\
        printf("%-20s ", item->svalue);\
        print_statmetric_2d\
        printf("\n");\
    }

    printf("%-8s ", "fold");
    printf("%-8s ", "wsize");
    printf("%-15s ", "thchooser");
    printf("%-25s ", "psetitem");
    printf("%-20s ", "psetitem value");
    DGAFOR(cl) {
        printf("  min[%d]     max[%d]     avg[%d]  ", cl, cl, cl);
        printf("    ");
    }
    printf("\n");
    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                multi_psetitem(prin_stat);
            }
        }
    }

    #define fprint_statmetric_csv\
        DGAFOR(cl) {\
            fprintf(file, ",%d,%d,", (int) (item->train[cl].min * 100), (int) (item->test[cl].min * 100));\
            fprintf(file, "%d,%d,", (int) (item->train[cl].max * 100), (int) (item->test[cl].max * 100));\
            fprintf(file, "%d,%d", (int) (item->train[cl].avg * 100), (int) (item->test[cl].avg * 100));\
        }

    #define fprint_stat_csv(NAME)\
    for (size_t idxpsetitem = 0; idxpsetitem < psetitems.NAME.number; idxpsetitem++) {\
        StatByPSetItemValue* item = &stats.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser].by ## NAME._[idxpsetitem];\
        fprintf(file, "%ld,", idxfold);\
        fprintf(file, "%ld,", trainer->tb2->wsizes._[idxwsize].value);\
        fprintf(file, "%s,", trainer->thchoosers._[idxthchooser].name);\
        fprintf(file, "%s,", item->name);\
        fprintf(file, "%s", item->svalue);\
        fprint_statmetric_csv;\
        fprintf(file, "\n");\
    }

    FILE* file = fopen("stat.csv", "w");
    fprintf(file, "fold,wsize,thchooser,psetitem,psetitemvalue");
    DGAFOR(cl) {
        fprintf(file, ",min[%d] train,min[%d] test,max[%d] train,max[%d] test,avg[%d] train, avg[%d] test", cl, cl, cl, cl, cl, cl);
    }
    fprintf(file, "\n");
    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                multi_psetitem(fprint_stat_csv);
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

    FORBY(stat.by, fold) {
        FORBY(stat.by, wsize) {
            FORBY(stat.by, thchooser) {
                multi_psetitem(free_psetitem);
            }
            FREEMANY(stat.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser);
        }
        FREEMANY(stat.by.byfold._[idxfold].bywsize);
    }
    FREEMANY(stat.by.byfold);
}