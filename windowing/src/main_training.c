
#include "main_training.h"

#include "detect.h"
// #include "logger.h"
#include "performance_defaults.h"
#include "stat.h"
#include "trainer.h"

#include <errno.h>

void _main_training_csv(char fpath[PATH_MAX], RTrainer trainer) {
    FILE* file_csv;

    file_csv = fopen(fpath, "w");
    if (file_csv == NULL) {
        LOG_ERROR("Impossible to open file (%s): %s\n", file_csv, strerror(errno));
        return;
    }

    TrainerBy* by = &trainer->by;

    RTB2D tb2d = trainer->tb2d;

    fprintf(file_csv, "caos,caos,caos,wsize,pset,pset,pset,pset,pset,pset,pset,pset,train,train,train,test,test,test\n");
    fprintf(file_csv, "fold,try,k,wsize,ninf,pinf,nn,windowing,wl_rank,wl_value,nx_eps_incr,thchooser,0,1,2,0,1,2\n");

    BY_FOR(trainer->by, fold) {
        BY_FOR(trainer->by, try) {
            BY_FOR3(trainer->by, fold, try, split) {
                BY_FOR(trainer->by, thchooser) {
                    BY_FOR(trainer->by, config) {
                        TrainerBy_config* result = &BY_GET5(trainer->by, fold, try, split, thchooser, config);

                        fprintf(file_csv, "%ld,", idxfold);
                        fprintf(file_csv, "%ld,", idxtry);
                        fprintf(file_csv, "%ld,", idxsplit);
                        fprintf(file_csv, "%ld,", tb2d->tb2w->wsize);
                        fprintf(file_csv, "%f,", tb2d->tb2w->configsuite.configs._[idxconfig].ninf);
                        fprintf(file_csv, "%f,", tb2d->tb2w->configsuite.configs._[idxconfig].pinf);
                        fprintf(file_csv, "%s,", NN_NAMES[tb2d->tb2w->configsuite.configs._[idxconfig].nn]);
                        fprintf(file_csv, "%s,", WINDOWING_NAMES[tb2d->tb2w->configsuite.configs._[idxconfig].windowing]);
                        fprintf(file_csv, "%ld,", tb2d->tb2w->configsuite.configs._[idxconfig].wl_rank);
                        fprintf(file_csv, "%f,", tb2d->tb2w->configsuite.configs._[idxconfig].wl_value);
                        fprintf(file_csv, "%f,", tb2d->tb2w->configsuite.configs._[idxconfig].nx_epsilon_increment);
                        fprintf(file_csv, "%s", trainer->thchoosers._[idxthchooser].name);

                        DGAFOR(cl) {
                            fprintf(file_csv, ",%1.4f", DETECT_TRUERATIO(result->best_train, cl));
                        }
                        DGAFOR(cl) {
                            fprintf(file_csv, ",%1.4f", DETECT_TRUERATIO(result->best_test, cl));
                        }
                        fprintf(file_csv, "\n");
                    }
                }
            }
        }
    }
    fclose(file_csv);
}

RTrainer main_training_generate(char rootdir[DIR_MAX], RTB2D tb2d, MANY(Performance) thchoosers) {
    char trainerdir[DIR_MAX];
    RTrainer trainer;

    LOG_DEBUG("Start training...");

    trainer = trainer_run2(tb2d, thchoosers, trainerdir);

    // Stat stat = stat_run(trainer, parameters_toignore, fpathstatcsv);
    // stat_free(stat);

    return trainer;
}

RTrainer main_training_load(char dirpath[DIR_MAX]) {
    return NULL;
}

void main_training_stat(RTrainer trainer) {
    ParameterRealmEnabled pre;
    memset(pre, 0, sizeof(ParameterRealmEnabled));

    for (size_t idxpd = 0; idxpd < N_PARAMETERS; idxpd++) {
        const size_t pd_n = trainer->tb2d->tb2w->configsuite.pr[idxpd].number;
        MANY_INIT(pre[idxpd], pd_n, int);
        for (size_t idxparameter = 0; idxparameter < pd_n; idxparameter++) {
            pre[idxpd]._[idxparameter] = 1;
            trainer->tb2d->tb2w->configsuite.pr[idxpd]._[idxparameter].disabled = 0;
        }
    }

    char csvpath[PATH_MAX];
    io_path_concat(trainer->rootdir, "stat.csv", csvpath);
    Stat stat = stat_run(trainer, pre, csvpath);

    stat_free(stat);

    for (size_t idxpd = 0; idxpd < N_PARAMETERS; idxpd++) {
        MANY_FREE(pre[idxpd]);
    }
}