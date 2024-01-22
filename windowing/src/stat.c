#include "stat.h"

#include "dataset.h"
#include "io.h"
// #include "logger.h"
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

Stat stat_run(RTrainer trainer, ParameterRealmEnabled parameterrealmenabled, char csvpath[PATH_MAX]) {

    Stat stats;
    TrainerBy* by = &trainer->by;
    ConfigSuite* cs = &trainer->tb2d->tb2w->configsuite;

    BY_SETN(stats.by, fold, trainer->tb2d->n.fold);
    BY_SETN(stats.by, thchooser, trainer->thchoosers.number);


    #define init_statmetric(NAME) {\
        sm->NAME[cl].min = DBL_MAX;\
        sm->NAME[cl].max = - DBL_MAX;\
        sm->NAME[cl].avg = 0;\
        sm->NAME[cl].std = 0;\
        sm->NAME[cl].count = 0;\
    }

    MANY_INIT(stats.by.byfold, trainer->tb2d->n.fold, StatBy_fold);
    BY_FOR(stats.by, fold) {
        MANY_INIT(BY_GET(stats.by, fold).bythchooser, trainer->thchoosers.number, StatBy_thchooser);
        BY_FOR(stats.by, thchooser) {
            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                MANY_INIT(BY_GET2(stats.by, fold, thchooser)[pp], cs->pr[pp].number, StatByPSetItemValue);

                for (size_t idxparameter = 0; idxparameter < cs->pr[pp].number; idxparameter++) {
                    if (0 == cs->pr[pp]._[idxparameter].disabled) {
                        StatByPSetItemValue* sm;
                        sm = &STAT_IDX(stats, idxfold, idxthchooser, pp, idxparameter);
                        DGAFOR(cl) {
                            init_statmetric(train);
                            init_statmetric(test);
                        }
                    }
                }
            }
        }
    }
    #undef init_statmetric


    #define update_statmetric(NAME) \
    {\
        const double value = DETECT_TRUERATIO(result->best_ ## NAME, cl);\
        if (sm->NAME[cl].min > value) sm->NAME[cl].min = value;\
        if (sm->NAME[cl].max < value) sm->NAME[cl].max = value;\
        sm->NAME[cl].avg += value;\
        sm->NAME[cl].count++;\
    }
    BY_FOR(stats.by, fold) {
        BY_FOR(trainer->by, try) {
            BY_FOR3(trainer->by, fold, try, split) {
                if (BY_GET2(trainer->by, fold, try).isok == 0) {
                    LOG_DEBUG("Skipping splits fold%ld, try%ld", idxfold, idxtry);
                    continue;
                }
                BY_FOR(stats.by, thchooser) {
                    BY_FOR(trainer->by, config) {
                        TrainerBy_config* result;
                        Config* config;

                        config = &cs->configs._[idxconfig];
                        
                        result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);

                        DGAFOR(cl) {
                            int sum = result->best_train.windows[cl][0] + result->best_train.windows[cl][1];
                            if (sum == 0) {
                                LOG_ERROR("No windows fold#%ld try#%ld split#%ld thchooser#%s config#%ld for %ld-class: %5ld\t%5ld",
                                    idxfold, idxtry, idxsplit, trainer->thchoosers._[idxthchooser].name, idxconfig,
                                    cl
                                );
                            }
                        }

                        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                            StatByPSetItemValue* sm;
                            sm = &STAT_IDX(stats, idxfold, idxthchooser, pp, config->parameters[pp]->index);
                            DGAFOR(cl) {
                                update_statmetric(train);
                                update_statmetric(test);
                            }
                        }
                    }
                }
            }
        }
    }
    #undef update_statmetric


    #define finalize_statmetric(NAME) DGAFOR(cl) { sm->NAME[cl].avg /= sm->NAME[cl].count; }
    BY_FOR(stats.by, fold) {
        BY_FOR(stats.by, thchooser) {
            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                for (size_t idxparameter = 0; idxparameter < cs->pr[pp].number; idxparameter++) {
                    if (cs->pr[pp]._[idxparameter].disabled == 0) {
                        StatByPSetItemValue* sm;
                        sm = &STAT_IDX(stats, idxfold, idxthchooser, pp, idxparameter);
                        finalize_statmetric(train);
                        finalize_statmetric(test);
                    }
                }
            }
        }
    }
    #undef finalize_statmetric


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
    char formats[N_PARAMETERS][10] = {
        "%7.2f",
        "%7.2f",
        "%s",
        "%ld",
        "%7.2f",
        "%s",
        "%7.2f"
    };
    BY_FOR(stats.by, fold) {
        BY_FOR(stats.by, thchooser) {
            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                for (size_t idxparameter = 0; idxparameter < cs->pr[pp].number; idxparameter++) {\
                    if (cs->pr[pp]._[idxparameter].disabled) continue;\
                    StatByPSetItemValue* item;
                    item = &STAT_IDX(stats, idxfold, idxthchooser, pp, idxparameter);

                    printf("%-8ld ", idxfold);
                    printf("%-8ld ", trainer->tb2d->tb2w->wsize);
                    printf("%-15s ", trainer->thchoosers._[idxthchooser].name);
                    printf("%-25s ", parameters_definition[pp].name);
                    {  
                        char str[20];
                        parameters_definition[pp].print(cs->pr[pp]._[idxparameter], 7, str);
                        printf("%-20s ", str);
                    }
                    print_statmetric_2d
                    printf("\n");
                }
            }
        }
    }

    #define fprint_statmetric_csv\
        DGAFOR(cl) {\
            fprintf(file, ",%d,%d,", (int) (item->train[cl].min * 100), (int) (item->test[cl].min * 100));\
            fprintf(file, "%d,%d,", (int) (item->train[cl].max * 100), (int) (item->test[cl].max * 100));\
            fprintf(file, "%d,%d", (int) (item->train[cl].avg * 100), (int) (item->test[cl].avg * 100));\
        }

    FILE* file = fopen(csvpath, "w");
    fprintf(file, "fold,wsize,thchooser,psetitem,psetitemvalue");
    DGAFOR(cl) {
        fprintf(file, ",min[%d] train,min[%d] test,max[%d] train,max[%d] test,avg[%d] train, avg[%d] test", cl, cl, cl, cl, cl, cl);
    }
    fprintf(file, "\n");

    char csv_formats[N_PARAMETERS][10] = {
        "%f",
        "%f",
        "%s",
        "%ld",
        "%f",
        "%s",
        "%f"
    };
    BY_FOR(stats.by, fold) {
        BY_FOR(stats.by, thchooser) {
            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                for (size_t idxparameter = 0; idxparameter < cs->pr[pp].number; idxparameter++) {\
                    StatByPSetItemValue* item;
                    item = &STAT_IDX(stats, idxfold, idxthchooser, pp, idxparameter);

                    fprintf(file, "%ld,", idxfold);
                    fprintf(file, "%ld,", trainer->tb2d->tb2w->wsize);
                    fprintf(file, "%s,", trainer->thchoosers._[idxthchooser].name);
                    fprintf(file, "%s,", item->name);
                    {  
                        char str[20];
                        parameters_definition[pp].print(cs->pr[pp]._[idxparameter], 0, str);
                        // printf("%-20s ", str);
                    }
                    fprint_statmetric_csv;
                    fprintf(file, "\n");
                }
            }
        }
    }

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
    BY_FOR(stat.by, fold) {
        BY_FOR(stat.by, thchooser) {
            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                MANY_FREE(stat.by.byfold._[idxfold].bythchooser._[idxthchooser][pp]);
            }
        }
        MANY_FREE(stat.by.byfold._[idxfold].bythchooser);
    }
    MANY_FREE(stat.by.byfold);
}