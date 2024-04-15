/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main file of the project
 *
 *        Created:  03/24/2016 19:40:56
 *
 *         Author:  Gustavo Pantuza
 *   Organization:  Software Community
 *
 * ============================================================================
 */

#include "args.h"
#include "common.h"
#include "configsuite.h"
#include "colors.h"
#include "gatherer2.h"
#include "io.h"
// #include "logger.h"
#include "parameter_generator.h"
#include "wqueue.h"
#include "performance_defaults.h"
#include "source.h"
#include "stratosphere.h"
#include "windowing.h"
#include "windowmc.h"
#include "windowfold.h"
#include "windowsplit.h"
#include "windowsplitdetection.h"
#include "windowzonedetection.h"

#include <assert.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

ConfigSuite configsuite;
MANY(Performance) windowing_thchooser;

enum BO {
    BO_LOAD_OR_GENERATE,
    BO_TEST
};

char DGA_CLASSES[N_DGACLASSES][200] = {
    "no-infection",
    "generic-malware",
    "dga-malware",
    "octo_rat",
    "nukesped_rat",
    "realtimespy_spyware",
    "evilquest_ransomware",
    "pwnrig_cryptominer,human_attacker",
    "pwnrig_cryptominer",
    "kinsing_malware",
    "lupper_worm",
    "nanocore",
    "nanocore,rhadamanthys",
    "redlinestealer",
    "agenttesla",
    "lockbit_ransomware,agenttesla",
    "remcosrat",
    "dns_exfiltrations",
    "human_attack",
    "trickbot_malware",
    "lockbit_ransomware"
};

MANY(Performance) _main_training_performance() {
    MANY(Performance) performances;

    performances.number = 0;
    performances._ = NULL;

    MANY_INIT(performances, 10, Performance);

    int i = 0;
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_1];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_05];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_F1SCORE_01];
    // performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_TPR2];
    performances._[i++] = performance_defaults[PERFORMANCEDEFAULTS_FPR];

    performances.number = i;

    return performances;
}

int main (int argc, char* argv[]) {
    setbuf(stdout, NULL);

    // cwin = initscr();
    raw();

#ifdef IO_DEBUG
    __io__debug = 0;
#endif

#ifdef LOGGING
    logger_initFileLogger("log/log.txt", 100 * 1024 * 1024, 5);
    // logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_TRACE);
    logger_autoFlush(5000);
#endif

    char rootdir[PATH_MAX];

    char dataset[20];
    ParameterGenerator pg;
    int configs_tiny;
    
    {
        configs_tiny = 1;
        // strcpy(dataset, "CTU-13");
        strcpy(dataset, "CTU-SME-13");
    
        if (configs_tiny) {
            pg = parametergenerator_default_tiny();
        } else {
            pg = parametergenerator_default();
        }
    }

    WSize wsize = 50;
    if (QM_MAX_SIZE < (wsize * 3)) {
        printf("Error: wsize too large\n");
        exit(0);
    }
    
    if (argc == 2) {
        sprintf(rootdir, "%s", argv[1]);
    } else {
        sprintf(rootdir, "/home/princio/Desktop/results/dns2_2/%s/%ld/%s/", dataset, wsize, configs_tiny ? "tiny" : "full"); //, wsize, nsources, (max_configs ? max_configs : pg.max_size));
    }

    io_setdir(rootdir);
    
    int compute = 1;
    
    g2_init();

    if (g2_io_call(G2_WING, IO_READ)) {
        printf("Computing windowing...\n");
        configsuite_generate(pg);

        stratosphere_add(dataset, 0);

        windowing_apply(wsize);

        g2_io_call(G2_WING, IO_WRITE);
    } else {
        printf("Loaded windowing...\n");

    }

    __MANY many = g2_array(G2_WING);
    for (size_t w = 0; w < many.number; w++) {
        RWindowing windowing = (RWindowing) many._[w];
        printf("id=%3d\tday=%d\tfnreqmax=%5ld\twnum=%5ld\t%s\n", windowing->source->id, windowing->source->day, windowing->source->fnreq_max, windowing->window0many->number, DGA_CLASSES[windowing->source->wclass.mc]);
    }

    {
        __MANY many = g2_array(G2_W0MANY);
        for (size_t i = 0; i < many.number; i++) {
            RWindow0Many w0many = (RWindow0Many) many._[i];
            assert(w0many->number);
            assert(w0many->_[0].applies.number);
            assert(w0many->_[0].applies._);
            for (size_t w = 0; w < w0many->number; w++) {
                assert(w0many->_[w].n_message > 0);
                for (size_t c = 0; c < configsuite.configs.number; c++) {
                    assert(w0many->_[w].applies._[c].wcount);
                    // could be zero due to windowing=windowing_r
                }
            }
        }
    }

    int split = 0;
    if (compute) {
        if (split == 0) {
            void* context = windowzonedetection_start();
            windowzonedetection_wait(context);
            g2_io_call(G2_DETECTION, IO_WRITE);
        } else {
            RWindowMC windowmc;

            windowmc = g2_insert_alloc_item(G2_WMC);
            windowmc_init(windowmc);
            windowmc_buildby_windowing(windowmc);

            WindowSplitConfig config = { 
                .how = WINDOWSPLIT_HOW_BY_DAY,
                .day = 1
            };

            if (strcmp(dataset, "CTU-SME-13") == 0) {
                windowsplit_createby_day(1);
            } else {
                RWindowMC windowmc;
                windowmc = (RWindowMC) g2_insert_alloc_item(G2_WMC);
                windowmc_init(windowmc);
                windowmc_buildby_windowing(windowmc);
                for (int k = 1; k <= 10; k++) {
                    windowsplit_createby_portion(windowmc, k, 10);
                }
            }

            {
                __MANY many = g2_array(G2_W0MANY);
                for (size_t i = 0; i < many.number; i++) {
                    RWindow0Many w0many = (RWindow0Many) many._[i];
                    assert(w0many->number);
                    assert(w0many->_[0].applies.number);
                    assert(w0many->_[0].applies._);
                    for (size_t w = 0; w < w0many->number; w++) {
                        assert(w0many->_[w].n_message > 0);
                    }
                }
            }
        }
        g2_io_all(IO_WRITE);
    } else {
        g2_io_all(IO_READ);
    }

    {
        FILE* fp = fopen("/tmp/detections.csv", "w");
        __MANY many = g2_array(G2_DETECTION);
        for (size_t d = 0; d < many.number; d++) {
            Detection* detection = (Detection*) many._[d];
            // windowzonedetection_print(detection);
            windowzonedetection_csv(fp, detection);
        }
        fclose(fp);
    }
    {
        MANY(StatDetectionCountZone) stat[N_PARAMETERS];

        memset(stat, 0, N_PARAMETERS * sizeof(MANY(StatDetectionCountZone)));

        detection_stat(stat);

        printf("\n\n--- Printing parameters statistics:\n\n");
        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            for (size_t p = 0; p < configsuite.realm[pp].number; p++) {
                printf("Parameter <%s> value \"", parameters_definition[pp].name);
                parameters_print(pp, &configsuite.realm[pp]._[p], 0);
                printf("\"\n\n");

                DGAFOR(cl) {
                    if (!strcmp(DGA_CLASSES[cl], "freqmax=0")) {
                        continue;
                    }
                    printf("%s\n", DGA_CLASSES[cl]);

                    {
                        printf(" (all) ");
                        printf("(%5u) ", stat[pp]._[p].dn.all.detections_involved[cl]);
                        printf("(%5u) | ", stat[pp]._[p].llr.all.detections_involved[cl]);
                        for (size_t z = 0; z < N_DETZONE; z++) {
                            char substrings[10][100];

                            if (stat[pp]._[p].dn.all._[z][cl].avg_denominator == 0) {
                                sprintf(substrings[0], "%5s", "-");
                                sprintf(substrings[2], "%5s", "-");
                                sprintf(substrings[4], "%5s", "-");
                            } else {
                                sprintf(substrings[0], "%5u", stat[pp]._[p].dn.all._[z][cl].min);
                                sprintf(substrings[2], "%5u", stat[pp]._[p].dn.all._[z][cl].max);
                                sprintf(substrings[4], "%5.0f", stat[pp]._[p].dn.all._[z][cl].avg);
                            }

                            if (stat[pp]._[p].llr.all._[z][cl].avg_denominator == 0) {
                                sprintf(substrings[1], "%-5s", "-");
                                sprintf(substrings[3], "%-5s", "-");
                                sprintf(substrings[5], "%-5s", "-");
                            } else {
                                sprintf(substrings[1], "%-5u", stat[pp]._[p].llr.all._[z][cl].min);
                                sprintf(substrings[3], "%-5u", stat[pp]._[p].llr.all._[z][cl].max);
                                sprintf(substrings[5], "%-5.0f", stat[pp]._[p].llr.all._[z][cl].avg);
                            }

                            for (size_t i = 0; i < 6; i+=2)
                            {
                                printf("%s ", substrings[i]);
                                printf("%s   ", substrings[i + 1]);
                            }
                            
                            printf(" | ");
                        }
                        printf("\n");
                    }

                    for (size_t day = 0; day < 7; day++) {
                        if (stat[pp]._[p].dn.days[day].detections_involved[cl] + stat[pp]._[p].llr.days[day].detections_involved[cl]) {
                            printf("(day%ld) ", day);
                            
                            printf("(%5u) ", stat[pp]._[p].dn.days[day].detections_involved[cl]);
                            printf("(%5u) | ", stat[pp]._[p].llr.days[day].detections_involved[cl]);
                            for (size_t z = 0; z < N_DETZONE; z++) {
                                char substrings[10][100];

                                if (stat[pp]._[p].dn.days[day]._[z][cl].avg_denominator == 0) {
                                    sprintf(substrings[0], "%5s", "-");
                                    sprintf(substrings[2], "%5s", "-");
                                    sprintf(substrings[4], "%5s", "-");
                                } else {
                                    sprintf(substrings[0], "%5u", stat[pp]._[p].dn.days[day]._[z][cl].min);
                                    sprintf(substrings[2], "%5u", stat[pp]._[p].dn.days[day]._[z][cl].max);
                                    sprintf(substrings[4], "%5.0f", stat[pp]._[p].dn.days[day]._[z][cl].avg);
                                }

                                if (stat[pp]._[p].llr.days[day]._[z][cl].avg_denominator == 0) {
                                    sprintf(substrings[1], "%-5s", "-");
                                    sprintf(substrings[3], "%-5s", "-");
                                    sprintf(substrings[5], "%-5s", "-");
                                } else {
                                    sprintf(substrings[1], "%-5u", stat[pp]._[p].llr.days[day]._[z][cl].min);
                                    sprintf(substrings[3], "%-5u", stat[pp]._[p].llr.days[day]._[z][cl].max);
                                    sprintf(substrings[5], "%-5.0f", stat[pp]._[p].llr.days[day]._[z][cl].avg);
                                }

                                for (size_t i = 0; i < 6; i+=2)
                                {
                                    printf("%s ", substrings[i]);
                                    printf("%s   ", substrings[i + 1]);
                                }
                                
                                printf(" | ");
                            }
                            printf("\n");
                        }
                    }
                    printf("\n");
                }
            }
        }

        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            MANY_FREE(stat[pp]);
        }
    }


    for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
        for (size_t p = 0; p < configsuite.realm[pp].number; p++) {
            printf("%20s\t", parameters_definition[pp].name);
            printf("%5ld\t", configsuite.realm[pp]._[p].index);
            parameters_print(pp, &configsuite.realm[pp]._[p], 0);
            printf("\n");
        }
    }


    for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
        Config* config = &configsuite.configs._[idxconfig];
        for (size_t pp = 0; pp < N_PARAMETERS; pp++) {
            printf("%20s\t", parameters_definition[pp].name);
            printf("%5ld\t", config->parameters[pp]->index);
            parameters_print(pp, config->parameters[pp], 0);
            printf("\n");
        }
    }

    if (0) {
        FILE* fp = fopen("/tmp/csv.csv", "w");
        __MANY many;
        many = g2_array(G2_W0MANY);
        for (size_t i = 0; i < many.number; i++) {
            RWindow0Many window0many = (RWindow0Many) many._[i];
            IndexMC count;
            memset(&count, 0, sizeof(IndexMC));
            count.all = window0many->number;
            count.binary[window0many->windowing->source->wclass.bc] = window0many->number;
            count.multi[window0many->windowing->source->wclass.mc] = window0many->number;
            fprintf(fp, "w0many,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        many = g2_array(G2_WMANY);
        for (size_t i = 0; i < many.number; i++) {
            RWindowMany windowmany = (RWindowMany) many._[i];
            IndexMC count = windowmany_count(windowmany);
            fprintf(fp, "wmany,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        many = g2_array(G2_WMC);
        for (size_t i = 0; i < many.number; i++) {
            RWindowMC windowmc;
            IndexMC count;

            windowmc = (RWindowMC) many._[i];
        
            count = windowmc_count(windowmc);
            fprintf(fp, "windowmc,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        many = g2_array(G2_WSPLIT);
        for (size_t i = 0; i < many.number; i++) {
            RWindowSplit windowsplit;
            IndexMC count;

            windowsplit = (RWindowSplit) many._[i];
            count = windowmany_count(windowsplit->train);

            fprintf(fp, "split-train,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");

            count = windowmc_count(windowsplit->test);
            fprintf(fp, "split-test,");
            fprintf(fp, "%ld,", i);
            fprintf(fp, "%ld,", count.all);
            BINFOR(bc) fprintf(fp, "%ld,", count.binary[bc]);
            DGAFOR(mc) fprintf(fp, "%ld,", count.multi[mc]);
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

    void* context = windowsplitdetection_start();
    windowsplitdetection_wait(context);

if (compute) {
    g2_io_all(IO_WRITE);
}

    g2_free_all();

    #ifdef LOGGING
    logger_close();
    #endif

    return 0;
}
