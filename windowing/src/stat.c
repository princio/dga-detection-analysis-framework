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

void _stat_init_ratio(StatMetricRatio*smr) {
    smr->min = DBL_MAX;
    smr->max = - DBL_MAX;
    smr->avg = 0;
    smr->std = 0;
}

void _stat_init_truefalse(StatMetricPN* smtf) {
   smtf->min = SIZE_MAX;
   smtf->max = 0;
   smtf->avg = 0;
   smtf->std = 0;
   smtf->count = 0;
}

void _stat_init(StatMetric sm[N_DGACLASSES]) {
    DGAFOR(cl) {
        _stat_init_ratio(&sm[cl].ratio);
        _stat_init_truefalse(&sm[cl].alarms.windowed);
        FOR_DNBAD {
            _stat_init_truefalse(&sm[cl].alarms.unwindowed[idxdnbad]);
        }
        _stat_init_truefalse(&sm[cl].alarms.sources);
        sm[cl].count = 0;
    }
}

void _stat_update_ratio(StatMetricRatio* smr, const double ratio) {
    if (smr->min > ratio) smr->min = ratio;
    if (smr->max < ratio) smr->max = ratio;
    smr->avg += ratio;
}

void _stat_update_pn(StatMetricPN* smtf, const DetectionPN pn) {
    if (smtf->min_over == 0) {
        smtf->min_over = pn[1] + pn[0];
        smtf->max_over = pn[1] + pn[0];
    }
    if (smtf->min > pn[1]) {
        smtf->min = pn[1];
        smtf->min_over = pn[1] + pn[0];
    }
    if (smtf->max < pn[1]) {
        smtf->max = pn[1];
        smtf->max_over = pn[1] + pn[0];
    }
    smtf->avg += pn[1];
    smtf->avg_over += pn[1] + pn[0];
}

void _stat_update(Detection* det, StatMetric sm[N_DGACLASSES]) {
    DGAFOR(cl) {
        const double tr = PN_TRUERATIO(det->windows, cl);

        _stat_update_ratio(&sm[cl].ratio, tr);

        _stat_update_pn(&sm[cl].alarms.windowed, det->windows[cl]);

        DetectionPN pn_sources = { 0, 0 };
        for (size_t idxsource = 0; idxsource < 50; idxsource++) {
            if (det->sources[cl][idxsource][0] + det->sources[cl][idxsource][1] == 0) {
                break;
            }
            pn_sources[det->sources[cl][idxsource][1] > 0]++;
        }
        _stat_update_pn(&sm[cl].alarms.sources, pn_sources);

        FOR_DNBAD {
            DetectionPN pn_alarms;
            pn_alarms[0] = det->dn_count[cl] - det->alarms[cl][idxdnbad];
            pn_alarms[1] = det->alarms[cl][idxdnbad];
            _stat_update_pn(&sm[cl].alarms.unwindowed[idxdnbad], pn_alarms);
        }
    
        sm[cl].count++;
    }
}

void _stat_finalize(StatMetric sm[N_DGACLASSES]) {
    DGAFOR(cl) {
        if (sm[cl].count == 0) {
            continue;
        }
        sm[cl].ratio.avg /= sm[cl].count;
        FOR_DNBAD {
            sm[cl].alarms.unwindowed[idxdnbad].avg /= sm[cl].count;
            sm[cl].alarms.unwindowed[idxdnbad].avg_over /= sm[cl].count;
        }
        sm[cl].alarms.windowed.avg /= sm[cl].count;
        sm[cl].alarms.windowed.avg_over /= sm[cl].count;

        sm[cl].alarms.sources.avg /= sm[cl].count;
        sm[cl].alarms.sources.avg_over /= sm[cl].count;
    }
}

const char* append_k(double value, char* format) {
    static char tmp[100];
    sprintf(tmp, format, value);
    strcat(tmp, "k");
    return tmp;
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
        if (idxfold == 0) {
            DGAFOR(cl) {
                BY_GET(stats.by, fold).wcount[cl].train = trainer->tb2d->bytry._[0].byfold._[idxfold].splits._[0].train->windows.multi[cl].number;
                BY_GET(stats.by, fold).wcount[cl].test = trainer->tb2d->bytry._[0].byfold._[idxfold].splits._[0].test->windows.multi[cl].number;
            }
        }
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
    PF_ASK(stdout, -15, "dn count #0");
    PF_ASK(stdout, -15, "dn count #1");
    PF_ASK(stdout, -15, "dn count #2");
    printf("\n\t\t\t");
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

                    DGAFOR(cl) {
                        PF_ASK_CENTER(stdout, 25, "%8ld %-8ld", BY_GET(stats.by, fold).wcount[cl].train, BY_GET(stats.by, fold).wcount[cl].test);
                    }
                    
                    printf("\n");


                    PF_ASK(stdout, 30, " ");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 45, "tpr");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 90, "sources w/w");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 90, "alarms w/w");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");
                
                    FOR_DNBAD {
                        PF_ASK_CENTER(stdout, 120, "alarms %4.3f", WApplyDNBad_Values[idxdnbad]);
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");
                    }
                    printf("\n");

                    PF_ASK(stdout, 30, " ");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 15, "min"); // tpr
                    PF_ASK_CENTER(stdout, 15, "max"); // tpr
                    PF_ASK_CENTER(stdout, 15, "avg"); // tpr
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 30, "min");
                    PF_ASK_CENTER(stdout, 30, "max");
                    PF_ASK_CENTER(stdout, 30, "avg");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");

                    PF_ASK_CENTER(stdout, 30, "min");
                    PF_ASK_CENTER(stdout, 30, "max");
                    PF_ASK_CENTER(stdout, 30, "avg");
                    PF_ASK(stdout, 2, " ");
                    PF_ASK(stdout, 1, "|");
                    PF_ASK(stdout, 2, " ");
                
                    FOR_DNBAD {
                        PF_ASK_CENTER(stdout, 40, "min");
                        PF_ASK_CENTER(stdout, 40, "max");
                        PF_ASK_CENTER(stdout, 40, "avg");
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");
                    }
                    printf("\n");
                    
                    DGAFOR(cl) {
                        PF_ASK(stdout, 30, "class=%d", cl);
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        PF_ASK_CENTER(stdout, 15, "%3d %-3d", (int) (item->train[cl].ratio.min * 100), (int) (item->test[cl].ratio.min * 100));
                        PF_ASK_CENTER(stdout, 15, "%3d %-3d", (int) (item->train[cl].ratio.max * 100), (int) (item->test[cl].ratio.max * 100));
                        PF_ASK_CENTER(stdout, 15, "%3.0d %-3.0d", (int) (item->train[cl].ratio.avg * 100), (int) (item->test[cl].ratio.avg * 100));
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        PF_ASK_CENTER(stdout, 30, "%6ld/%-6ld %6ld/%-6ld", item->train[cl].alarms.sources.min, item->train[cl].alarms.sources.min_over, item->test[cl].alarms.sources.min, item->test[cl].alarms.sources.min_over);
                        PF_ASK_CENTER(stdout, 30, "%6ld/%-6ld %6ld/%-6ld", item->train[cl].alarms.sources.max, item->train[cl].alarms.sources.max_over, item->test[cl].alarms.sources.max, item->test[cl].alarms.sources.max_over);
                        PF_ASK_CENTER(stdout, 30, "%6.2f %-6.2f", item->train[cl].alarms.sources.avg, item->test[cl].alarms.sources.avg);
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        PF_ASK_CENTER(stdout, 30, "%6ld/%-6ld %6ld/%-6ld", item->train[cl].alarms.windowed.min, item->train[cl].alarms.windowed.min_over, item->test[cl].alarms.windowed.min, item->test[cl].alarms.windowed.min_over);
                        PF_ASK_CENTER(stdout, 30, "%6ld/%-6ld %6ld/%-6ld", item->train[cl].alarms.windowed.min, item->train[cl].alarms.windowed.min_over, item->test[cl].alarms.windowed.min, item->test[cl].alarms.windowed.min_over);
                        PF_ASK_CENTER(stdout, 30, "%6.2f/%6.2f %-6.2f/%6.2f",
                            item->train[cl].alarms.windowed.avg,
                            item->train[cl].alarms.windowed.avg_over,
                            item->test[cl].alarms.windowed.avg,
                            item->test[cl].alarms.windowed.avg_over
                        );
                        PF_ASK(stdout, 2, " ");
                        PF_ASK(stdout, 1, "|");
                        PF_ASK(stdout, 2, " ");

                        FOR_DNBAD {
                            PF_ASK_CENTER(stdout, 40, "%8ld/%-8s %8ld/%-8s",
                                item->train[cl].alarms.unwindowed[idxdnbad].min,
                                append_k((double) item->train[cl].alarms.unwindowed[idxdnbad].min_over/1000, "%.1f"),
                                item->test[cl].alarms.unwindowed[idxdnbad].min,
                                append_k((double) item->test[cl].alarms.unwindowed[idxdnbad].min_over/1000, "%.1f")
                            );
                            PF_ASK_CENTER(stdout, 40, "%8ld/%-8s %8ld/%-8s",
                                item->train[cl].alarms.unwindowed[idxdnbad].max,
                                append_k((double) item->train[cl].alarms.unwindowed[idxdnbad].max_over/1000, "%.1f"),
                                item->test[cl].alarms.unwindowed[idxdnbad].max,
                                append_k((double) item->test[cl].alarms.unwindowed[idxdnbad].max_over/1000, "%.1f")
                            );
                            PF_ASK_CENTER(stdout, 40, "%8.0f/%-8s %8.0f/%-8s",
                                item->train[cl].alarms.unwindowed[idxdnbad].avg,
                                append_k((double) item->train[cl].alarms.unwindowed[idxdnbad].avg_over / 1000, "%.1f"),
                                item->test[cl].alarms.unwindowed[idxdnbad].avg,
                                // item->test[cl].alarms.unwindowed[idxdnbad].avg_over / 1000
                                append_k((double) item->test[cl].alarms.unwindowed[idxdnbad].avg_over / 1000, "%.1f")
                            );
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

    // BY_FOR1(*trainer->tb2d, try) {
    //     BY_FOR2(*trainer->tb2d, try, fold) {
    //         for (size_t idxsplit = 0; idxsplit < BY_GET2(*trainer->tb2d, try, fold).splits.number; idxsplit++) {
    //             DatasetSplit ds = BY_GET2(*trainer->tb2d, try, fold).splits._[idxsplit];

    //             DGAFOR(cl) {
    //                 printf("%7ld %-7ld\t", ds.train->windows.multi[cl].number, ds.test->windows.multi[cl].number);
    //             }
    //             printf("\n");
    //         }
    //     }
    // }

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
                        PF_ASK(file, -15, "%-3d %-d", (int) (item->train[cl].ratio.min * 100), (int) (item->test[cl].ratio.min * 100));
                        PF_ASK(file, -15, "%-3d %-d", (int) (item->train[cl].ratio.max * 100), (int) (item->test[cl].ratio.max * 100));
                        PF_ASK(file, -15, "%-3d %-d", (int) (item->train[cl].ratio.avg * 100), (int) (item->test[cl].ratio.avg * 100));

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