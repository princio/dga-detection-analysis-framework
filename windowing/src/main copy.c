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


#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "colors.h"
#include "dn.h"
#include "parameters.h"
#include "persister.h"

#include <stdint.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* for ntohl/htonl */
#include <netinet/in.h>
#include <arpa/inet.h>

#define N_WINDOWS(FNREQ_MAX, B) ((FNREQ_MAX + 1) / B + ((FNREQ_MAX + 1) % B > 0)) // +1 because it starts from 0


char WINDOWING_NAMES[3][10] = {
    "QUERY",
    "RESPONSE",
    "BOTH"
};

char NN_NAMES[11][10] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "ICANN",
    "NONE",
    "PRIVATE",
    "TLD"
};

struct WindowDebug {
    int wnum;
    int wcount;
};

typedef struct Overrall {
    int window_size;
    int nwindows;
    int nw_infected_05;
    int nw_infected_09;
    int nw_infected_099;
    int nw_infected_0999;
    int correct_predictions;
} Overrall;

void print_pi(MetricParameters *pi, int z) {
    printf("%d\n", z);
    printf(" Window size:  %d\n", pi->wsize);
    printf("Whitelisting:  (%d, %f)\n", pi->whitelisting.rank, pi->whitelisting.value);
    printf("          NN:  %s\n", NN_NAMES[pi->nn]);
    printf("   Windowing:  %s\n\n", WINDOWING_NAMES[pi->windowing]);
}

int main (int argc, char* argv[]) {
    setbuf(stdout, NULL);
srand
    /* Read command line options */
    options_t options;
    options_parser(argc, argv, &options);

#ifdef DEBUG
    fprintf(stdout, BLUE "Command line options:\n" NO_COLOR);
    fprintf(stdout, BROWN "help: %d\n" NO_COLOR, options.help);
    fprintf(stdout, BROWN "version: %d\n" NO_COLOR, options.version);
    fprintf(stdout, BROWN "use colors: %d\n" NO_COLOR, options.use_colors);
    fprintf(stdout, BROWN "filename: %s\n" NO_COLOR, options.file_name);
#endif

    /* Do your magic here :) */


    // persister_test();

    // exit(0);

    char root_dir[100];
    sprintf(root_dir, "/home/princio/Desktop/exps/exp_230918");
    struct stat st = {0};
    if (stat(root_dir, &st) == -1) {
        mkdir(root_dir, 0700);
    }

    int N_PIS;
    PSets pis;

    int n_pcaps_all;
    int n_pcaps_dga01;
    Capture* pcaps;
    Capture** pcaps_all;
    Capture** pcaps_dga01;

    int N_WSIZE = 5;
    int wsize[] = { 10, 50, 100, 500, 2500 };

    int debug_pcaps_number;

    parameters_generate(root_dir, &pis, &N_PIS);
    
    stratosphere_pcaps(root_dir, &pcaps, &n_pcaps_all);

    pcaps_all = calloc(sizeof(Capture*), n_pcaps_all);
    n_pcaps_dga01 = 0;
    for (int i = 0; i < n_pcaps_all; i++) {
        pcaps_all[i] = &pcaps[i];
        n_pcaps_dga01 += pcaps[i].infected_type != INFECTED;
    }

    pcaps_dga01 = calloc(sizeof(CapturePtr), n_pcaps_dga01);
    {
        int cursor = 0;
        for (int i = 0; i < n_pcaps_all; i++) {
            if (pcaps[i].infected_type != INFECTED) {
                pcaps_dga01[cursor++] = &pcaps[i];
            }
        }
    }

    printf("PCAP NOT-INFECTED AND INFECTED: %d\n", n_pcaps_all);
    printf("PCAP NOT-INFECTED AND DGA: %d\n", n_pcaps_dga01);
    

    /**
     *  ONLY FOR DEBUG
     */
    int pcap_subset = 1;
    Captures* PCAPS = pcap_subset ? pcaps_dga01 : pcaps_all;
    const int N_PCAPS = pcap_subset ? n_pcaps_dga01 : n_pcaps_all;

    debug_pcaps_number = N_PCAPS;
    /**
     *  
     */

    // qui abbiamo tutti i PCAP e tutti i parameter-set

    const int N_METRICS = N_PIS / N_WSIZE; 
    printf("    N_PIS\t%d\n", N_PIS);
    printf("N_METRICS\t%d\n", N_METRICS);
    

    /**
     * @brief Qui inizializzazione, per ogni wsize, uno struct AllWindows il quale detiene un array di puntatori a tutte le finestr
     * 
     */
    AllWindows all_windows[N_WSIZE];
    memset(all_windows, 0, N_WSIZE * sizeof(AllWindows));
    for (int w = 0; w < N_WSIZE; ++w) {
        all_windows[w].wsize = wsize[w];
        for (int i = 0; i < debug_pcaps_number; ++i) {
            Capture *pcap = PCAPS[i];
            int n_windows =  N_WINDOWS(pcap->fnreq_max, wsize[w]);

            all_windows[w].total += n_windows;
            if (pcap->infected_type) {
                all_windows[w].total_positives += n_windows;
            } else {
                all_windows[w].total_negatives += n_windows;
            }
        }

        all_windows[w].windows = calloc(all_windows[w].total, sizeof(Window*));
        all_windows[w].windows_negatives = calloc(all_windows[w].total_negatives, sizeof(Window*));
        all_windows[w].windows_positives = calloc(all_windows[w].total_positives, sizeof(Window*));
    }


    PCAPWindowingPtr pcapwindowings_byPcap_byWSIZE[debug_pcaps_number][N_WSIZE];
    AllWindowsCursor all_windows_cursor[N_WSIZE];
    memset(all_windows_cursor, 0, N_WSIZE * sizeof(AllWindowsCursor));
    for (int i = 0; i < debug_pcaps_number; ++i) {
        Capture *pcap = PCAPS[i];
        
        printf("PCAP %ld having dga=%d and nrows=%ld\n", pcap->id, pcap->infected_type, pcap->nmessages);

        PCAPWindowingPtr*pcapwindowings_byWSIZE = pcapwindowings_byPcap_byWSIZE[i];
        for (int w = 0; w < N_WSIZE; ++w) {
            int nwindows = N_WINDOWS(pcap->fnreq_max, wsize[w]);

            PCAPWindowingPtr*pcapwindowing = &pcapwindowings_byWSIZE[w];

            pcapwindowing->pcap_id = pcap->id;
            pcapwindowing->infected = pcap->infected_type;
            pcapwindowing->nwindows = nwindows;
            pcapwindowing->_ = calloc(nwindows, sizeof(Window));
            pcapwindowing->wsize = wsize[w];

            for (int r = 0; r < nwindows; r++) {
                Window* window = &pcapwindowing->_[r];
                int pi_i;

                all_windows[w].windows[all_windows_cursor[w].all++] = window;
                if (pcap->infected_type) {
                    all_windows[w].windows_positives[all_windows_cursor[w].positives++] = window;
                } else {
                    all_windows[w].windows_negatives[all_windows_cursor[w].negatives++] = window;
                }

                window->wsize = wsize[w];
                
                window->wnum = r;
                window->infected = pcap->infected_type;
                window->parent_id = pcap->id;
                window->nmetrics = N_METRICS;
                window->metrics = calloc(N_METRICS, sizeof(WindowMetricSets));

                pi_i = 0;
                for (int k = 0; k < N_PIS; ++k) {
                    if (pis[k].wsize == wsize[w] ) {
                        window->metrics[pi_i].pi_id = pis[k].id;
                        window->metrics[pi_i].pi = &pis[k];
                        ++pi_i;

                        MetricParameters* pi = &pis[k];

                        if ((i + w + r) == 0 && k % N_WSIZE == 0) {
                            printf("%10d\t", pi_i);
                            printf("%10d\t", pi->wsize);
                            printf("(%10d, %10.5f)\t", pi->whitelisting.rank, pi->whitelisting.value);
                            printf("%10d\t", pi->nn);
                            printf("%10d\n", pi->windowing);
                        }
                    }
                    // if ((i + w + r) == 0 && k % N_WSIZE == 0) {
                    //     Pi* pi = &pis[k];
                    //     printf("%10d\t", k / N_WSIZE);
                    //     printf("%10d\t", pi->wsize);
                    //     printf("(%10d, %10.5f)\t", pi->whitelisting.rank, pi->whitelisting.value);
                    //     printf("%10s\t", NN_NAMES[pi->nn]);
                    //     printf("%10s\t\n", WINDOWING_NAMES[pi->windowing]);
                    // }
                }
            }
        }
        printf(" Window size, Whitelisting, NN, Windowing\n");

        /*
        
        all_windows: a (_ x 3) vector where 3 is the number of chosen window sizes.
        total_pcaps_windows: a (1 x 3) vector where 3 is the number of chosen window sizes and the element is the number of windows for this size.

        */

        stratosphere_procedure(root_dir, pcap, pcapwindowings_byWSIZE, N_WSIZE);
    }

    {
        FILE*fp = fopen("pcap.csv", "w");
        fprintf(fp, "wnum,pcap\n");
        for (int p = 0; p < N_PCAPS; p++)
        {
            PCAPWindowingPtr pcapwindowing = &pcapwindowings_byPcap_byWSIZE[p][2];
            for (int i = 0; i < pcapwindowing->nwindows; i++)
            {
                fprintf(fp, "%d,%d\n", pcapwindowing->_[i].wnum, pcapwindowing->_[i].pcap_id);
            }
        }
        fclose(fp);
    }

    {
        FILE*fp = fopen("allwindows.csv", "w");
        fprintf(fp, "wnum,pcap\n");
        AllWindows* all_windows_2500 = &all_windows[2];
        for (int i = 0; i < all_windows_2500->total_positives; i++)
        {
            fprintf(fp, "%d,%d\n", all_windows_2500->__positives[i]->wnum, all_windows_2500->__positives[i]->pcap_id);
        }
        fclose(fp);
    }

    char exp_name[100];
    char exp_dir[200];
    {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(exp_name, "test_%s_%d%02d%02d_%02d%02d%02d", pcap_subset ? "dga02" : "all", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        
        sprintf(exp_dir, "%s/%s", root_dir, exp_name);
        struct stat st = {0};
        if (stat(exp_dir, &st) == -1) {
            mkdir(exp_dir, 0700);
        }
    }


    char path[strlen(exp_dir) + 50];
    sprintf(path, "%s/test.csv", exp_dir);
    FILE* fp = fopen(path, "w");

    int KFOLD = 50;
    int N_SPLITS = 10;
    double FPR[N_WSIZE][KFOLD];
    double TPR[N_WSIZE][KFOLD];
    memset(FPR, 0, sizeof(double) * KFOLD * N_WSIZE);
    memset(TPR, 0, sizeof(double) * KFOLD * N_WSIZE);
    
    double AVG_FPR[N_SPLITS][N_WSIZE];
    double AVG_TPR[N_SPLITS][N_WSIZE];
    memset(AVG_FPR, 0, sizeof(double) * N_SPLITS * N_WSIZE);
    memset(AVG_TPR, 0, sizeof(double) * N_SPLITS * N_WSIZE);

    double AVG_FPR_PER_METRIC[N_SPLITS][N_WSIZE][N_METRICS];
    double AVG_TPR_PER_METRIC[N_SPLITS][N_WSIZE][N_METRICS];
    memset(AVG_FPR_PER_METRIC, 0, sizeof(double) * N_SPLITS * N_WSIZE * N_METRICS);
    memset(AVG_TPR_PER_METRIC, 0, sizeof(double) * N_SPLITS * N_WSIZE * N_METRICS);

    double split_percentages[10] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7 };
    int split_nwindows[N_SPLITS][3];

    memset(split_nwindows, 0, N_SPLITS * 3 * sizeof(int));

    printf("split");
    for (int w = 0; w < N_WSIZE; ++w) {
        printf("\t%18d", wsize[w]);
    }
    printf("\n");
    for (int p = 0; p < N_SPLITS; ++p) {
        double split_percentage = split_percentages[p];
        for (int k = 0; k < KFOLD; ++k) {
            for (int w = 0; w < N_WSIZE; ++w) {
                // printf("\n ---- WSIZE = %d ----\n\n", wsize[w]);

                WindowingDataset wd;
                int32_t tot_n = all_windows[w].total_negatives;
                int32_t tot_p = all_windows[w].total_positives;

                wd.train.n.number = floor(tot_n * split_percentage);
                wd.train.n.windows = calloc(wd.train.n.number, sizeof(Window*));

                wd.test.n.number = tot_n - wd.train.n.number;
                wd.test.n.windows = calloc(wd.test.n.number, sizeof(Window*));

                wd.train.p.number = floor(tot_p * split_percentage);
                wd.train.p.windows = calloc(wd.train.p.number, sizeof(Window*));

                wd.test.p.number = tot_p - wd.train.p.number;
                wd.test.p.windows = calloc(wd.test.p.number, sizeof(Window*));

                split_nwindows[p][w] = wd.train.n.number;

                extract_wd(all_windows[w].windows_negatives, &wd.train.n, &wd.test.n);
                extract_wd(all_windows[w].windows_positives, &wd.train.p, &wd.test.p);

                
                int nmetrics = wd.train.n.windows[0]->nmetrics;
                double logit_max[nmetrics];
                for (int m = 0; m < nmetrics; m++) logit_max[m] = - DBL_MAX;

                for (int i = 0; i < wd.train.n.number; i++)
                {
                    Window* window = wd.train.n.windows[i];

                    for (int m = 0; m < nmetrics; m++)
                    {
                        double _l = window->metrics[m].logit;
                        if (logit_max[m] < _l) logit_max[m] = _l;

                        if (p == 0 && i == 0) {
                            MetricParameters* pi = window->metrics[m].pi;
                            printf("%10d\t", m);
                            printf("%10d\t", pi->wsize);
                            printf("(%10d, %10.5f)\t", pi->whitelisting.rank, pi->whitelisting.value);
                            printf("%10d\t", pi->nn);
                            printf("%10d\n", pi->windowing);
                            // printf("%10s\t", NN_NAMES[pi->nn]);
                            // printf("%10s\n", WINDOWING_NAMES[pi->windowing]);
                        }
                    }
                }


                int cm[nmetrics][4];
                memset(cm, 0, sizeof(int) * nmetrics * 4);
                for (int i = 0; i < wd.test.n.number; i++)
                {
                    Window* window = wd.test.n.windows[i];

                    for (int m = 0; m < nmetrics; m++)
                    {
                        if (window->metrics[m].logit >= logit_max[m]) ++cm[m][1];
                        else ++cm[m][0];
                    }
                }

                for (int i = 0; i < wd.test.p.number; i++)
                {
                    Window* window = wd.test.p.windows[i];

                    for (int m = 0; m < nmetrics; m++)
                    {
                        if (window->metrics[m].logit < logit_max[m]) ++cm[m][2];
                        else ++cm[m][3];
                    }
                }

                double avg[4] = { 0, 0, 0, 0 };
                double fpr_avg_per_metric[N_PIS];
                double tpr_avg_per_metric[N_PIS];
                memset(fpr_avg_per_metric, 0, sizeof(double) * N_PIS);
                memset(tpr_avg_per_metric, 0, sizeof(double) * N_PIS);
                for (int m = 0; m < nmetrics; m++) {
                    fpr_avg_per_metric[m] = cm[m][1];
                    tpr_avg_per_metric[m] = cm[m][3];
                    
                    avg[0] += cm[m][0];
                    avg[1] += cm[m][1];
                    avg[2] += cm[m][2];
                    avg[3] += cm[m][3];
                }

                avg[0] /= nmetrics;
                avg[1] /= nmetrics;
                avg[2] /= nmetrics;
                avg[3] /= nmetrics;


                int nn = cm[0][0] + cm[0][1];
                int pp = cm[0][2] + cm[0][3];

                FPR[w][k] = avg[1] / nn;
                TPR[w][k] = avg[3] / pp;

                AVG_FPR[p][w] += avg[1] / nn;
                AVG_TPR[p][w] += avg[3] / pp;

                for (int m = 0; m < nmetrics; m++) {
                    AVG_FPR_PER_METRIC[p][w][m] += (fpr_avg_per_metric[m] / nn);
                    AVG_TPR_PER_METRIC[p][w][m] += (tpr_avg_per_metric[m] / pp);
                }
            }
        }

        for (int k = 0; k < KFOLD; ++k) {
            for (int w = 0; w < N_WSIZE; ++w) {
                fprintf(fp, "%2.2f,%3d,%5d,%8.3f,%8.3f\n", split_percentage, k, wsize[w], FPR[w][k], TPR[w][k]);
                // AVG_FPR[p][w] += FPR[w][k];
                // AVG_TPR[p][w] += TPR[w][k];
            }
        }
        printf("%4.2f", split_percentage);
        for (int w = 0; w < N_WSIZE; ++w) {
            printf("\t%6d\t", split_nwindows[p][w]);
            printf("%6.4f/%6.4f", AVG_FPR[p][w] * 100 / KFOLD, AVG_TPR[p][w] * 100 / KFOLD);
        }
        printf("\n");
    }

    fclose(fp);


    for (int p = 0; p < N_SPLITS; ++p) {
        double split_percentage = split_percentages[p];
        printf("\n%5.2f\n", split_percentage);
        for (int w = 0; w < N_WSIZE; ++w) {
            printf("%10d\n", wsize[w]);
            for (int m = 0; m < N_METRICS; ++m) {
                MetricParameters* pi = &pis[m * N_WSIZE + w];
                printf("%10d\t", m / N_WSIZE);
                printf("%10d\t", pi->wsize);
                printf("(%10d, %10.5f)\t", pi->whitelisting.rank, pi->whitelisting.value);
                printf("%10s\t", NN_NAMES[pi->nn]);
                printf("%10s\t", WINDOWING_NAMES[pi->windowing]);
                printf("%7.4f/%7.4f\n", AVG_FPR_PER_METRIC[p][w][m] * 100 / KFOLD, AVG_TPR_PER_METRIC[p][w][m] * 100 / KFOLD);
            }
        }
    }

    // printf("%5s,%5s,%8d", "", "", overrall.nwindows);
    // printf("%8d,%8d,%8d,%8d,", overrall.nw_infected_05, overrall.nw_infected_09, overrall.nw_infected_099, overrall.nw_infected_0999);
    // printf("%8d\n", overrall.correct_predictions);

    return EXIT_SUCCESS;
}