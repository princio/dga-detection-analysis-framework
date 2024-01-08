#include "stat.h"

#include "dataset.h"
#include "io.h"
#include "logger.h"
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

Stat stat_run(RTrainer trainer, ParameterRealmEnabled parameterrealmenabled, char fpath[PATH_MAX]) {

    Stat stats;
    TrainerBy* by = &trainer->by;

    INITBY_N(stats.by, fold, trainer->tb2->dataset.n.fold);
    INITBY_N(stats.by, wsize, trainer->tb2->dataset.n.wsize);
    INITBY_N(stats.by, thchooser, trainer->thchoosers.number);


    #define init_statmetric(NAME) {\
        sm->NAME[cl].min = DBL_MAX;\
        sm->NAME[cl].max = - DBL_MAX;\
        sm->NAME[cl].avg = 0;\
        sm->NAME[cl].std = 0;\
        sm->NAME[cl].count = 0;\
    }
    INITBYFOR(stats.by, stats.by, fold, StatBy) {
        INITBYFOR(stats.by, GETBY(stats.by, fold), wsize, StatBy) {
            INITBYFOR(stats.by, GETBY2(stats.by, fold, wsize), thchooser, StatBy) {
                for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                    INITMANY(GETBY3(stats.by, fold, wsize, thchooser)[pp], parameterrealm[pp].number, StatByPSetItemValue);
                    for (size_t idxparameter = 0; idxparameter < parameterrealm[pp].number; idxparameter++) {
                        if (0 == parameterrealm[pp]._[idxparameter].disabled) {
                            StatByPSetItemValue* sm;
                            sm = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, pp, idxparameter);
                            DGAFOR(cl) {
                                init_statmetric(train);
                                init_statmetric(test);
                            }
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
    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                FORBY(trainer->by, apply) {
                    FORBY(trainer->by, try) {
                        for (size_t k = 0; k < GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits.number; k++) {
                            TrainerBy_splits* result;
                            Config* config;

                            config = trainer->tb2->applies._[idxapply].config;
                            
                            result = &GETBY5(trainer->by, wsize, apply, fold, try, thchooser).splits._[k];

                            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                                StatByPSetItemValue* sm;
                                sm = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, pp, config->parameters[pp]->index);
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
    }
    #undef update_statmetric


    #define finalize_statmetric(NAME) DGAFOR(cl) { sm->NAME[cl].avg /= sm->NAME[cl].count; }
    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                    for (size_t idxparameter = 0; idxparameter < parameterrealm[pp].number; idxparameter++) {
                        if (parameterrealm[pp]._[idxparameter].disabled == 0) {
                            StatByPSetItemValue* sm;
                            sm = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, pp, idxparameter);
                            finalize_statmetric(train);
                            finalize_statmetric(test);
                        }
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
    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                    for (size_t idxparameter = 0; idxparameter < parameterrealm[pp].number; idxparameter++) {\
                        if (parameterrealm[pp]._[idxparameter].disabled) continue;\
                        StatByPSetItemValue* item;
                        item = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, pp, idxparameter);

                        printf("%-8ld ", idxfold);
                        printf("%-8ld ", trainer->tb2->wsizes._[idxwsize].value);
                        printf("%-15s ", trainer->thchoosers._[idxthchooser].name);
                        printf("%-25s ", parameters_definition[pp].name);
                        {  
                            char str[20];
                            parameters_definition[pp].print(parameterrealm[pp]._[idxparameter], 7, str);
                            printf("%-20s ", str);
                        }
                        print_statmetric_2d
                        printf("\n");
                    }
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

    FILE* file = fopen(fpath, "w");
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
    FORBY(stats.by, fold) {
        FORBY(stats.by, wsize) {
            FORBY(stats.by, thchooser) {
                for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                    for (size_t idxparameter = 0; idxparameter < parameterrealm[pp].number; idxparameter++) {\
                        StatByPSetItemValue* item;
                        item = &STAT_IDX(stats, idxfold, idxwsize, idxthchooser, pp, idxparameter);

                        fprintf(file, "%ld,", idxfold);
                        fprintf(file, "%ld,", trainer->tb2->wsizes._[idxwsize].value);
                        fprintf(file, "%s,", trainer->thchoosers._[idxthchooser].name);
                        fprintf(file, "%s,", item->name);
                        {  
                            char str[20];
                            parameters_definition[pp].print(parameterrealm[pp]._[idxparameter], 0, str);
                            // printf("%-20s ", str);
                        }
                        fprint_statmetric_csv;
                        fprintf(file, "\n");
                    }
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
    FORBY(stat.by, fold) {
        FORBY(stat.by, wsize) {
            FORBY(stat.by, thchooser) {
                for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                    FREEMANY(stat.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser._[idxthchooser][pp]);
                }
            }
            FREEMANY(stat.by.byfold._[idxfold].bywsize._[idxwsize].bythchooser);
        }
        FREEMANY(stat.by.byfold._[idxfold].bywsize);
    }
    FREEMANY(stat.by.byfold);
}