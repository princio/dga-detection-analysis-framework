
#include "main_training.h"

#include "detect.h"
// #include "logger.h"
#include "performance_defaults.h"
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

    BY_FOR(trainer->by, config) {
        BY_FOR(trainer->by, fold) {
            BY_FOR(trainer->by, try) {
                BY_FOR(trainer->by, thchooser) {
                    for (size_t k = 0; k < BY_GET4(trainer->by, config, fold, try, thchooser).splits.number; k++) {
                        TrainerBy_splits* result = &BY_GET4(trainer->by, config, fold, try, thchooser).splits._[k];

                        fprintf(file_csv, "%ld,", idxfold);
                        fprintf(file_csv, "%ld,", idxtry);
                        fprintf(file_csv, "%ld,", k);
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

void _main_training_print(RTrainer trainer) {

    FILE* file_csv = fopen("results.csv", "w");

    TrainerBy* by = &trainer->by;

    RTB2D tb2d = trainer->tb2d;


    fprintf(file_csv, ",,,,,,train,train,train,test,test,test");
    fprintf(file_csv, "fold,try,k,wsize,apply,thchooser,0,1,2,0,1,2");


    BY_FOR(trainer->by, config) {
        BY_FOR(trainer->by, fold) {
            BY_FOR(trainer->by, try) {
                BY_FOR(trainer->by, thchooser) {
                    for (size_t k = 0; k < BY_GET4(trainer->by, config, fold, try, thchooser).splits.number; k++) {
                        TrainerBy_splits* result = &BY_GET4(trainer->by, config, fold, try, thchooser).splits._[k];

                        {
                            char headers[4][50];
                            char header[210];
                            size_t h_idx = 0;

                            sprintf(headers[h_idx++], "%3ld", idxfold);
                            sprintf(headers[h_idx++], "%3ld", idxtry);
                            sprintf(headers[h_idx++], "%3ld", k);
                            sprintf(headers[h_idx++], "%5ld", tb2d->tb2w->wsize);
                            sprintf(headers[h_idx++], "%3ld", idxconfig);
                            sprintf(headers[h_idx++], "%12s", trainer->thchoosers._[idxthchooser].name);
                            sprintf(header, "%s,%s,%s,%s,", headers[0], headers[1], headers[2], headers[3]);
                            printf("%-30s ", header);
                        }

                        DGAFOR(cl) {
                            printf("%1.4f\t", DETECT_TRUERATIO(result->best_train, cl));
                        }
                        printf("|\t");
                        DGAFOR(cl) {
                            printf("%1.4f\t", DETECT_TRUERATIO(result->best_test, cl));
                        }
                        printf("\n");
                    }
                }
            }
        }
    }
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