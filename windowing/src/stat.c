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

#define PF_ASK_CENTER(FILE, ASK, ...) {\
    char __tmp__kk[200];\
    sprintf(__tmp__kk, __VA_ARGS__);\
    int __bb = fprintf(FILE, "%*s", (int) (ASK - strlen(__tmp__kk)) / 2, " ");\
    fprintf(FILE, "%*s", -1 * (int) (ASK - __bb), __tmp__kk);\
}
#define PF_ASK(FILE, ASK, ...) { char __tmp__kk[200]; sprintf(__tmp__kk, __VA_ARGS__); fprintf(FILE, "%*s", ASK, __tmp__kk); }
// #define PF_ASK(FILE, ASK, ...) PF_ASK_CENTER(FILE, ASK, __VA_ARGS__)

#define update_stat(stat, value) \
    if (stat.min > value) stat.min = value;\
    if (stat.max < value) stat.max = value;\
    stat.avg += value;

#define update_stat(stat, value) \
    if (stat.min > value) stat.min = value;\
    if (stat.max < value) stat.max = value;\
    stat.avg += value;

void _stat_init(StatMetric sm[N_DGACLASSES]) {
    DGAFOR(cl) {
        sm[cl].logit.min = DBL_MAX;
        sm[cl].logit.max = - DBL_MAX;
        sm[cl].logit.avg = 0;
        sm[cl].logit.std = 0;

        sm[cl].alarms.windowed.min = SIZE_MAX;
        sm[cl].alarms.windowed.max = 0;
        sm[cl].alarms.windowed.avg = 0;
        sm[cl].alarms.windowed.std = 0;
        sm[cl].alarms.windowed.count = 0;

        FOR_DNBAD {
            sm[cl].alarms.unwindowed[idxdnbad].min = SIZE_MAX;
            sm[cl].alarms.unwindowed[idxdnbad].max = 0;
            sm[cl].alarms.unwindowed[idxdnbad].avg = 0;
            sm[cl].alarms.unwindowed[idxdnbad].std = 0;
        }

        sm[cl].count = 0;
    }
}

void _stat_update(Detection* det, StatMetric sm[N_DGACLASSES]) {
    DGAFOR(cl) {
        const double tr = DETECT_TRUERATIO(*det, cl);
        const DetectionValue n_alarms = DETECT_WINDOW_ALARMS(*det, cl);

        update_stat(sm[cl].logit, tr);
        update_stat(sm[cl].alarms.windowed, n_alarms);

        FOR_DNBAD {
            update_stat(sm[cl].alarms.unwindowed[idxdnbad], det->alarms[cl][idxdnbad]);
        }
    
        sm[cl].count++;
    }
}

void _stat_finalize(StatMetric sm[N_DGACLASSES]) {
    DGAFOR(cl) {
        sm[cl].logit.avg /= sm[cl].count;
        FOR_DNBAD {
            sm[cl].alarms.unwindowed[idxdnbad].avg /= sm[cl].count;
        }
        sm[cl].alarms.windowed.avg /= sm[cl].count;
    }
}

Stat stat_run(RTrainer trainer, ParameterRealmEnabled parameterrealmenabled, char csvpath[PATH_MAX]) {

    Stat stats;
    TrainerBy* by = &trainer->by;
    ConfigSuite* cs = &trainer->tb2d->tb2w->configsuite;

    BY_SETN(stats.by, fold, trainer->tb2d->n.fold);
    BY_SETN(stats.by, thchooser, trainer->thchoosers.number);


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
                        _stat_init(sm->train);
                        _stat_init(sm->test);
                    }
                }
            }
        }
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
                            _stat_update(&result->best_train, sm->train);
                            _stat_update(&result->best_test, sm->test);
                        }
                    }
                }
            }
        }
    }

    BY_FOR(stats.by, fold) {
        BY_FOR(stats.by, thchooser) {
            for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
                for (size_t idxparameter = 0; idxparameter < cs->pr[pp].number; idxparameter++) {
                    if (cs->pr[pp]._[idxparameter].disabled == 0) {
                        StatByPSetItemValue* sm;
                        sm = &STAT_IDX(stats, idxfold, idxthchooser, pp, idxparameter);
                        _stat_finalize(sm->train);
                        _stat_finalize(sm->test);
                    }
                }
            }
        }
    }

    PF_ASK(stdout, -8, "fold");
    PF_ASK(stdout, -8, "wsize");
    PF_ASK(stdout, -10, "thchooser");
    PF_ASK(stdout, -10, "psetitem");
    PF_ASK(stdout, -15, "psetitem value");
    printf("\n\t\t\t");
    DGAFOR(cl) {
        PF_ASK(stdout, -10, "%d#count", cl);

        PF_ASK(stdout, -15, "%d#min logit", cl);
        PF_ASK(stdout, -15, "%d#max logit", cl);
        PF_ASK(stdout, -15, "%d#avg logit", cl);
        PF_ASK(stdout, -5, " ");

        PF_ASK(stdout, -20, "%d#min windowed", cl);
        PF_ASK(stdout, -20, "%d#max windowed", cl);
        PF_ASK(stdout, -20, "%d#avg windowed", cl);
        PF_ASK(stdout, -5, " ");
    
        FOR_DNBAD {
            PF_ASK(stdout, -15, "%d#min >%3.2f", cl, WApplyDNBad_Values[idxdnbad]);
            PF_ASK(stdout, -15, "%d#max >%3.2f", cl, WApplyDNBad_Values[idxdnbad]);
            PF_ASK(stdout, -15, "%d#avg >%3.2f", cl, WApplyDNBad_Values[idxdnbad]);
            PF_ASK(stdout, -5, " ");
        }
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

                    PF_ASK(stdout, -8, "%ld", idxfold);
                    PF_ASK(stdout, -8, "%ld", trainer->tb2d->tb2w->wsize);
                    PF_ASK(stdout, -10, "%s", trainer->thchoosers._[idxthchooser].name);
                    PF_ASK(stdout, -10, "%s", parameters_definition[pp].name);
                    {  
                        char str[20];
                        parameters_definition[pp].print(cs->pr[pp]._[idxparameter], 0, str);
                        PF_ASK(stdout, -15, "%s", str);
                    }
                    PF_ASK(stdout, -10, "%-8ld", item->train[0].count);
                    
                    printf("\n");


                    PF_ASK(stdout, 55, " ");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 45, "tpr");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 45, "alarms w/w");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");
                
                    FOR_DNBAD {
                        PF_ASK_CENTER(stdout, 45, "alarms %4.4f", WApplyDNBad_Values[idxdnbad]);
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");
                    }
                    printf("\n");

                    PF_ASK(stdout, 55, " ");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 15, "min");
                    PF_ASK_CENTER(stdout, 15, "max");
                    PF_ASK_CENTER(stdout, 15, "avg");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 15, "min");
                    PF_ASK_CENTER(stdout, 15, "max");
                    PF_ASK_CENTER(stdout, 15, "avg");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");
                
                    FOR_DNBAD {
                        PF_ASK_CENTER(stdout, 15, "min");
                        PF_ASK_CENTER(stdout, 15, "max");
                        PF_ASK_CENTER(stdout, 15, "avg");
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");
                    }
                    printf("\n");
                    
                    DGAFOR(cl) {
                        PF_ASK(stdout, 55, "class=%d", cl);
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        PF_ASK_CENTER(stdout, 15, "%3d %-3d", (int) (item->train[cl].logit.min * 100), (int) (item->test[cl].logit.min * 100));
                        PF_ASK_CENTER(stdout, 15, "%3d %-3d", (int) (item->train[cl].logit.max * 100), (int) (item->test[cl].logit.max * 100));
                        PF_ASK_CENTER(stdout, 15, "%3.0d %-3.0d", (int) (item->train[cl].logit.avg * 100), (int) (item->test[cl].logit.avg * 100));
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        PF_ASK_CENTER(stdout, 15, "%6ld %-6ld", item->train[cl].alarms.windowed.min, item->test[cl].alarms.windowed.min);
                        PF_ASK_CENTER(stdout, 15, "%6ld %-6ld", item->train[cl].alarms.windowed.max, item->test[cl].alarms.windowed.max);
                        PF_ASK_CENTER(stdout, 15, "%6.0f %-6.0f", item->train[cl].alarms.windowed.avg, item->test[cl].alarms.windowed.avg);
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        FOR_DNBAD {
                            PF_ASK_CENTER(stdout, 15, "%6ld %-6ld", item->train[cl].alarms.unwindowed[idxdnbad].min, item->test[cl].alarms.unwindowed[idxdnbad].min);
                            PF_ASK_CENTER(stdout, 15, "%6ld %-6ld", item->train[cl].alarms.unwindowed[idxdnbad].max, item->test[cl].alarms.unwindowed[idxdnbad].max);
                            PF_ASK_CENTER(stdout, 15, "%6.0f %-6.0f", item->train[cl].alarms.unwindowed[idxdnbad].avg, item->test[cl].alarms.unwindowed[idxdnbad].avg);
                            PF_ASK(stdout, 2, " ");
                            PF_ASK(stdout, 1, "|");
                            PF_ASK(stdout, 2, " ");
                        }
                        printf("\n");
                    }
                    
                    printf("\n");
                }
            }
        }
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
                    
                    DGAFOR(cl) {
                        PF_ASK(file, -15, "%-3d %-d", (int) (item->train[cl].logit.min * 100), (int) (item->test[cl].logit.min * 100));
                        PF_ASK(file, -15, "%-3d %-d", (int) (item->train[cl].logit.max * 100), (int) (item->test[cl].logit.max * 100));
                        PF_ASK(file, -15, "%-3d %-d", (int) (item->train[cl].logit.avg * 100), (int) (item->test[cl].logit.avg * 100));

                        PF_ASK(file, -20, "%-6ld %-ld", item->train[cl].alarms.windowed.min, item->test[cl].alarms.windowed.min);
                        PF_ASK(file, -20, "%-6ld %-ld", item->train[cl].alarms.windowed.max, item->test[cl].alarms.windowed.max);
                        PF_ASK(file, -20, "%-6.0f %-.0f", item->train[cl].alarms.windowed.avg, item->test[cl].alarms.windowed.avg);

                        FOR_DNBAD {
                            PF_ASK(file, -20, "%-6ld %-ld", item->train[cl].alarms.unwindowed[idxdnbad].min, item->test[cl].alarms.unwindowed[idxdnbad].min);
                            PF_ASK(file, -20, "%-6ld %-ld", item->train[cl].alarms.unwindowed[idxdnbad].max, item->test[cl].alarms.unwindowed[idxdnbad].max);
                            PF_ASK(file, -20, "%-6.0f %-.0f", item->train[cl].alarms.unwindowed[idxdnbad].avg, item->test[cl].alarms.unwindowed[idxdnbad].avg);
                        }
                    }

                    fprintf(file, "\n");
                }
            }
        }
    }

    fclose(file);

    return stats;
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