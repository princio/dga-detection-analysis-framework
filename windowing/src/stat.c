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

int stat_pset_ignore(TCPC(PSet) pset, TCPC(PSetByField) psetitems_to_ignore) {
    #define __IGNORE(NAME) for (size_t idxpsetitem = 0; idxpsetitem < psetitems_to_ignore->NAME.number; idxpsetitem++) {\
        int cmp = memcmp(&pset->NAME, &psetitems_to_ignore->NAME._[idxpsetitem], sizeof(NAME ## _t));\
        if (cmp == 0) {\
            return 1;\
        }\
    }

    multi_psetitem(__IGNORE);

    return 0;
}

Stat stat_run(RTrainer trainer, PSetByField psetitems_toignore, char fpath[PATH_MAX]) {

    Stat stats;
    PSetByField psetitems;
    TrainerBy* by = &trainer->by;


    #define psetitems_toignore(NAME)\
    printf(#NAME " [%ld/%ld]\t", psetitems_toignore.NAME.number, psetitems.NAME.number);\
    for (size_t idxfield = 0; idxfield < psetitems.NAME.number; idxfield++) {\
        for (size_t idxfield2ignore = 0; idxfield2ignore < psetitems_toignore.NAME.number; idxfield2ignore++) {\
            NAME ## _t NAME ## _value = psetitems_toignore.NAME._[idxfield2ignore];\
            int cmp = memcmp(&NAME ## _value, &psetitems.NAME._[idxfield], sizeof(NAME ## _t));\
            if (cmp == 0) {\
                printf("%ld-%" PSET_FMT(NAME) "\t", idxfield, PSET_FMT_PRE_2(NAME, psetitems.NAME._[idxfield]));\
                psetitems.NAME._[idxfield] = psetitems.NAME._[--psetitems.NAME.number];\
                idxfield--;\
                if (0 == psetitems.NAME.number) {\
                    printf("Warning: will ignore all due to ignoring all fields of %s.\n", #NAME);\
                    return stats;\
                }\
                break;\
            }\
        }\
    }\
    printf("\n");

    {
        MANY(PSet) psets;
        INITMANY(psets, trainer->tb2->applies.number, PSet);

        for (size_t idxapply = 0; idxapply < trainer->tb2->applies.number; idxapply++) {
            PSet* pset = &trainer->tb2->applies._[idxapply].pset;
            psets._[idxapply] = *pset;
        }
        
        psetitems = parameters_psets2byfields(&psets);

        FREEMANY(psets);
    }

    // printf("windowing"
    //     " [%ld/%ld]\t",
    //     psetitems_toignore.windowing.number, psetitems.windowing.number);
    // for (size_t idxfield = 0; idxfield < psetitems.windowing.number; idxfield++) {
    //     for (size_t idxfield2ignore = 0; idxfield2ignore < psetitems_toignore.windowing.number; idxfield2ignore++) {
    //         windowing_t windowing_value = psetitems_toignore.windowing._[idxfield2ignore];
    //         int cmp = memcmp(&windowing_value, &psetitems.windowing._[idxfield], sizeof(windowing_t));
    //         if (cmp == 0) {
    //             printf("%ld-%"
    //                 "s"
    //                 "\t",
    //                 idxfield, WINDOWING_NAMES[psetitems.windowing._[idxfield]]);
    //             psetitems.windowing._[idxfield] = psetitems.windowing._[--psetitems.windowing.number];
    //             idxfield--;
    //             if (0 == psetitems.windowing.number) {
    //                 printf("Warning: will ignore all due to ignoring all fields of %s.\n", "windowing");
    //                 return stats;
    //             }
    //             break;
    //         }
    //     }
    // }
    // printf("\n");


    multi_psetitem(psetitems_toignore);


    #define init_statmetric(NAME) DGAFOR(cl) DGAFOR(cl) {\
        sm->NAME[cl].min = DBL_MAX;\
        sm->NAME[cl].max = - DBL_MAX;\
        sm->NAME[cl].avg = 0;\
        sm->NAME[cl].std = 0;\
        sm->NAME[cl].count = 0;\
    }

    #define init_stat(NAME) \
    {\
        INITMANY(GETBY3(stats.by, fold, wsize, thchooser).by ## NAME, psetitems.NAME.number, StatByPSetItemValue);\
        for (size_t __idx = 0; __idx < psetitems.NAME.number; __idx++) {\
            StatByPSetItemValue* sm = &GETBY3(stats.by, fold, wsize, thchooser).by ## NAME._[__idx];\
            NAME ## _t NAME ## _value = psetitems.NAME._[__idx];\
            strcpy(sm->name, #NAME);\
            sprintf(sm->svalue, "%"PSET_FMT(NAME), PSET_FMT_PRE(NAME));\
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

    INITBY_N(stats.by, fold, trainer->tb2->dataset.n.fold);
    INITBY_N(stats.by, wsize, trainer->tb2->dataset.n.wsize);
    INITBY_N(stats.by, thchooser, trainer->thchoosers.number);

    INITBYFOR(stats.by, stats.by, fold, StatBy) {
        INITBYFOR(stats.by, GETBY(stats.by, fold), wsize, StatBy) {
            INITBYFOR(stats.by, GETBY2(stats.by, fold, wsize), thchooser, StatBy) {
                multi_psetitem(init_stat);
            }
        }
    }

    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                FORBY(trainer->by, apply) {
                    FORBY(trainer->by, try) {
                        for (size_t k = 0; k < GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits.number; k++) {
                            TrainerBy_splits* result;
                            result = &GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits._[k];
                            if (stat_pset_ignore(&trainer->tb2->applies._[idxapply].pset, &psetitems_toignore)) continue;
update_stat(ninf);
update_stat(pinf);
update_stat(nn);
update_stat(wl_rank);
update_stat(wl_value);
update_stat(windowing);
update_stat(nx_epsilon_increment);
                            // multi_psetitem(update_stat);
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

    FILE* file = fopen(fpath, "w");
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

    fclose(file);

    return stats;

#undef init_statmetric
#undef init_stat
#undef update_statmetric
#undef update_stat
#undef finalize_statmetric
#undef finalize_stat
#undef print_statmetric
#undef print_statmetric_2d
#undef prin_stat
#undef fprint_statmetric_csv
#undef fprint_stat_csv
#undef pc_free
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