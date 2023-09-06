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
#include "persister.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* for ntohl/htonl */
#include <netinet/in.h>
#include <arpa/inet.h>

#define N_WINDOWS(A,B) (A / B + (A % B > 0))

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

void parse_message(PGresult* res, int row, Message* message)
{
    message->fn_req = atoi(PQgetvalue(res, row, 0));
    message->value = atof(PQgetvalue(res, row, 1));
    message->logit = atof(PQgetvalue(res, row, 2));
    message->is_response = PQgetvalue(res, row, 3)[0] == 't';
    message->top10m = atoi(PQgetvalue(res, row, 4));
    message->dyndns = PQgetvalue(res, row, 5)[0] == 't';
}


void printdatastring(PGresult* res)
{
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        int ncols = PQnfields(res);
        int nrows = 5;//PQntuples(res);

        for(int r = 0; r < nrows; r++)
        {
            for (int c = 0; c < ncols; c++)
            {
                char* colname = PQfname(res, c);
                char* value = PQgetvalue(res, r, c);
                printf("%10s\t", colname);
                printf("%25s\t", value);

                Oid oid = PQftype(res, c);
                switch (PQftype(res, c)) {
                    case INT2OID:
                        printf("%20s\n", "INT2OID");
                    break;
                    case INT4OID:
                        printf("%20s\n", "INT4OID");
                    break;
                    case INT8OID: {
                        int64_t iptr = atol(value);
                        printf("%20ld\t", iptr);
                        printf("%20s[%zu]\n", "INT8OID", sizeof (int64_t));
                        break;
                    }
                    case FLOAT4OID: {
                        double f4ptr = atoi(value);
                        printf("%20f\t", f4ptr);
                        printf("%20s\n", "FLOAT4OID");
                        break;
                    }
                    case FLOAT8OID: {
                        double f8ptr = atof(value);
                        printf("%20f\t", f8ptr);
                        printf("%20s[%zu]\n", "FLOAT8OID", sizeof (double));
                        break;
                    }
                    case BOOLOID: {
                        int _bool = value[0] == 't';
                        printf("%20i\t", _bool);
                        printf("%20s\n", "BOOLOID");
                        break;
                    }
                    default:
                        printf("%20u\n", oid);
                }
            }

            puts("");
        }
    }
}

void CDPGclose_connection(PGconn* conn)
{
    PQfinish(conn);
}

PGconn* CDPGget_connection()
{
    PGconn *conn = PQconnectdb("postgresql://princio:postgres@localhost/dns");

    if (PQstatus(conn) == CONNECTION_OK)
    {
        puts("CONNECTION_OK\n");

        return conn;
    }
    
    fprintf(stderr, "CONNECTION_BAD %s\n", PQerrorMessage(conn));

    CDPGclose_connection(conn);

    return NULL;
}

/**
 * @brief It execute the windowing for the pcap and the number of windowing size chosen, in order to execute the pcap retrieve from
 * the database only one time for all the windowing sizes.
 * 
 * @param conn 
 * @param pcap 
 * @param windowings An array of PCAPWindowing struct where each element represent the windowing of the PCAP for a defined window size.
 * @param N_WINDOWING The number of window size we chose.
 */
void procedure(PGconn* conn, PCAP* pcap, PCAPWindowing windowings[], int N_WINDOWING)
{
    char sql[1000];
    int binary = 1;
    
    sprintf(sql,
            "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS FROM MESSAGES_%ld AS M "
            "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
            "JOIN DN AS DN ON M.DN_ID=DN.ID "
            "ORDER BY FN_REQ",
            pcap->id);

    // PGresult* resb = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, binary);
    // printdatabinary(resb);

    PGresult* res = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !binary);
    

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data\n");
        return;
    }

    int nrows = PQntuples(res);

    if (nrows != pcap->nmessages) {
        printf("Errorrrrrr:\t%d\t%ld\n", nrows, pcap->nmessages);
    }

    // struct WindowDebug *debug[N_WINDOWING];
    // for (size_t i = 0; i < N_WINDOWING; i++) {
    //     debug[i] = calloc(N_WINDOWS(pcap->nmessages, wsize[i]), sizeof(struct WindowDebug));
    // }
    

    for(int r = 0; r < nrows; r++) {
        Message message;

        parse_message(res, r, &message);

        for (int w0 = 0; w0 < N_WINDOWING; ++w0) {
            PCAPWindowing *windowing;
            int wnum;
            Window *window;
            
            windowing = &windowings[w0];

            wnum = (int) floor(message.fn_req / windowing->wsize);

            window = &windowing->windows[wnum];

            for (int w2 = 0; w2 < window->nmetrics; w2++) {
                double value, logit;
                WindowMetrics *metrics;
                Pi *pi;
                
                metrics = &window->metrics[w2];
                pi = metrics->pi;

                if (pi->windowing == WINDOWING_Q && message.is_response) {
                    continue;
                } else
                if (pi->windowing == WINDOWING_R && !message.is_response) {
                    continue;
                }

                if (wnum != window->wnum) {
                    printf("Errorrrrrr:\t%d\t%d\n", wnum, window->wnum);
                }

                logit = message.logit;
                value = message.value;

                if (message.top10m > 0 && message.top10m < pi->whitelisting.rank) {
                    value = 0;
                    logit = pi->whitelisting.value;
                }
                if (logit == INFINITY) {
                    logit = pi->infinite_values.pinf;
                } else
                    if (logit == (-1 * INFINITY)) {
                    logit = pi->infinite_values.ninf;
                }

                ++metrics->wcount;
                metrics->logit += logit;
                metrics->dn_bad_05 += value >= 0.5;
                metrics->dn_bad_09 += value >= 0.9;
                metrics->dn_bad_099 += value >= 0.99;
                metrics->dn_bad_0999 += value >= 0.999;

                if (pi->logit_range.min > metrics->logit) {
                    pi->logit_range.min = metrics->logit;
                }
                if (pi->logit_range.max < metrics->logit) {
                    pi->logit_range.max = metrics->logit;
                }
            }
        }

/*
        for (int pi_i = 0; pi_i < N_PIS; ++pi_i) {
            double value, logit;
            Pi *pi;
            // Overrall *overall;
            int wnum;
            Window *window;
            WindowMetrics *metrics;

            pi = &pis[pi_i];

            wnum = (int) floor(message.fn_req / pi->wsize);

            int wsize_index = -1;
            do {} while(wsize[++wsize_index] != pi->wsize);

            if (wsize_index == 2)// && wnum == 161 && pi_i == 26)
                printf("%d\t%d\twindows[%d][%d]->metrics[%d/%d]\t\t%d\t\t", message.fn_req, pi->wsize, wsize_index, wnum, pi_i, N_PIS, message.top10m);


            if (pi->windowing == WINDOWING_Q && message.is_response) {
                continue;
            } else
            if (pi->windowing == WINDOWING_R && !message.is_response) {
                continue;
            }

            debug[wsize_index][wnum].wcount++;


            window = &windows[wsize_index][wnum];
            metrics = &window->metrics[pi_i];

            if (wnum != window->wnum) {
                printf("Errorrrrrr:\t%d\t%d\n", wnum, window->wnum);
            }

            logit = message.logit;
            value = message.value;

            if (message.top10m > 0 && message.top10m < pi->whitelisting.rank) {
                value = 0;
                logit = pi->whitelisting.value;
            }
            if (logit == INFINITY) {
                logit = pi->infinite_values.pinf;
            } else
                if (logit == (-1 * INFINITY)) {
                logit = pi->infinite_values.ninf;
            }

            if (wsize_index == 2) //0 && wnum == 161 && pi_i == 26)
                printf("%f\n", logit);

            window->wsize += 1;
            window->wcount += 1;
            metrics->logit += logit;
            metrics->dn_bad_05 += value >= 0.5;
            metrics->dn_bad_09 += value >= 0.9;
            metrics->dn_bad_099 += value >= 0.99;
            metrics->dn_bad_0999 += value >= 0.999;

        }
*/        
        
        // for (int w = 0; w < N_WSIZE; ++w) {
        //     int nwindows = N_WINDOWS(pcap->nmessages, wsize[w]);
        //     for (int k = 0; k < nwindows; ++k) {
        //         printf("%d\t%d\n", k, debug[w][k].wcount);
        //     }
        // }

    }

    PQclear(res);
}

int has_been_extracted(int array[], int n, int dado) {
    for (int k = 0; k < n; ++k) {
        if (dado == array[k]) {
            return 1;
        }
    }
    return 0;
}

void extract(Window** windows, int n, Window* extracted[n]) {
    int i;
    int extractions[n];
    memset(extractions, 0, sizeof(int) * n);

    i = 0;
    srand(time(NULL));
    while (i < n) {
        int dado = rand() % n;

        if (has_been_extracted(extractions, i, dado)) continue;

        extractions[i] = dado;
        extracted[i] = windows[dado];
        ++i;
    }
}

void extract_wd(Window** windows, WindowingDataset3* wd3_train, WindowingDataset3* wd3_test) {
    int train_extracted[wd3_train->total];
    int i;

    memset(train_extracted, 0, sizeof(int) * wd3_train->total);

    srand(time(NULL));

    i = 0;
    while (i < wd3_train->total) {
        int dado = rand() % wd3_train->total;

        if (has_been_extracted(train_extracted, i, dado)) continue;

        train_extracted[i] = dado;
        wd3_train->windows[i] = windows[dado];
        ++i;
    }

    printf("%d\n", wd3_train->total);
    printf("%d\n", wd3_test->total);
    printf("%d\n\n", i);

    i = 0;
    for (int j = 0; j < (wd3_train->total + wd3_test->total); ++j) {
        int comodo = has_been_extracted(train_extracted, wd3_train->total, j);
        if (!comodo) {
            wd3_test->windows[i] = windows[j];
            ++i;
        }
    }

    if (i != wd3_test->total) {
        printf("Error %d != %d\n", i, wd3_test->total);
    }
}

int get_pcaps_number(PGconn* conn) {
    PGresult* res = PQexec(conn, "SELECT count(*) FROM pcap");
    int64_t rownumber = -1;

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        char* srownumber = PQgetvalue(res, 0, 0);

        rownumber = atoi(srownumber);
    }

    return rownumber;
}

void get_pcaps(PGconn* conn, PCAP** _pcaps, int* _N_PCAPS) {

    int N_PCAPS = get_pcaps_number(conn);

    PCAP* pcaps = calloc(N_PCAPS, sizeof(PCAP));

    PGresult* res = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r FROM pcap JOIN malware as mw ON malware_id = mw.id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        int nrows = PQntuples(res);

        for(int r = 0; r < nrows; r++)
        {
            int nmessages = 0;
            pcaps[r].id = atoi(PQgetvalue(res, r, 0));
            pcaps[r].infected = atoi(PQgetvalue(res, r, 1));
            pcaps[r].qr = atoi(PQgetvalue(res, r, 2));
            pcaps[r].q = atoi(PQgetvalue(res, r, 3));
            pcaps[r].r = atoi(PQgetvalue(res, r, 4));

            {
                char sql[1000];
                sprintf(sql,
                    "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
                    "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                    "JOIN DN AS DN ON M.DN_ID=DN.ID",
                    pcaps[r].id);

                PGresult* res_nrows = PQexec(conn, sql);
                if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                    printf("No data\n");
                } else {
                    nmessages = atoi(PQgetvalue(res_nrows, 0, 0));
                }
                PQclear(res_nrows);
            }
            pcaps[r].nmessages = nmessages;
        }
    }

    PQclear(res);

    *_pcaps = pcaps;
    *_N_PCAPS = N_PCAPS;
}

void print_pi(Pi *pi, int z) {
    printf("%d\n", z);
    printf(" Window size:  %d\n", pi->wsize);
    printf("Whitelisting:  (%d, %f)\n", pi->whitelisting.rank, pi->whitelisting.value);
    printf("          NN:  %s\n", NN_NAMES[pi->nn]);
    printf("   Windowing:  %s\n\n", WINDOWING_NAMES[pi->windowing]);
}

void generate_parameters(Pi** pis, int* N_PIS, int wsize[], int N_WSIZE) {
    NN nn[] = { NN_NONE, NN_TLD, NN_ICANN, NN_PRIVATE };

    Whitelisting whitelisting[] = {
        { .rank = 0, .value = 0 },
        { .rank = 1000, .value = -50 },
        { .rank = 100000, .value = -50 },
        { .rank = 1000000, .value = -10 },  
        { .rank = 1000000, .value = -50 } 
    };

    WindowingType windowing_types[] = {
        WINDOWING_Q,
        WINDOWING_R,
        WINDOWING_QR
    };

    InfiniteValues infinitevalues[] = {
        { .ninf = -20, .pinf = 20 }
    };

    size_t count_nn = sizeof(nn) / sizeof(NN);
    size_t count_wl = sizeof(whitelisting)/sizeof(Whitelisting);
    size_t count_wt = sizeof(windowing_types)/sizeof(WindowingType);
    size_t count_ws = N_WSIZE;
    size_t count_iv = sizeof(infinitevalues)/sizeof(InfiniteValues);


    printf("NN: %ld\n", count_nn);
    printf("WL: %ld\n", count_wl);
    printf("WT: %ld\n", count_wt);
    printf("WS: %ld\n", count_ws);
    printf("IV: %ld\n", count_iv);

    int n = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i3 = 0; i3 < count_ws; ++i3) {
                    for (size_t i4 = 0; i4 < count_iv; ++i4) {
                        ++n;
                    }
                }
            }
        }
    }

    printf("Number of parameters: %d\n", n);

    *N_PIS = n;
    Pi* _pis = calloc(n, sizeof(Pi));
    *pis = _pis;

    n = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i3 = 0; i3 < count_ws; ++i3) {
                    for (size_t i4 = 0; i4 < count_iv; ++i4) {
                        _pis[n].infinite_values = infinitevalues[i4];
                        _pis[n].nn = nn[i0];
                        _pis[n].whitelisting = whitelisting[i1];
                        _pis[n].windowing = windowing_types[i2];
                        _pis[n].wsize = wsize[i3];
                        _pis[n].id = n;

                        ++n;
                    }
                }
            }
        }
    }
}

int main (int argc, char* argv[]) {
    setbuf(stdout, NULL);

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
    sprintf(root_dir, "/home/princio/Desktop/exps/exp_230904");
    struct stat st = {0};
    if (stat(root_dir, &st) == -1) {
        mkdir(root_dir, 0700);
    }

    Pi* pis;
    int N_PIS;

    int N_WSIZE = 3;
    int wsize[] = { 100, 500, 2500 };


    {
        char __path[150];
        snprintf(__path, 150, "%s/parameters.bin", root_dir);

        int ret = read_Pi_file(__path, &pis, &N_PIS);

        if (!ret) {
            generate_parameters(&pis, &N_PIS, wsize, N_WSIZE);
            {
                int ret = write_Pi_file(__path, pis, N_PIS);

                if (!ret) printf("Pi-s write error\n");
            }
        }
    }

    PGconn* conn = CDPGget_connection();

    int N_PCAPS;
    PCAP* pcaps;

    // Overrall overralls[pcaps_number][N_PIS];

    {
        char __path[150];
        snprintf(__path, 150, "%s/pcaps.bin", root_dir);

        int ret = read_PCAP_file(__path, &pcaps, &N_PCAPS);

        if (!ret) {
            get_pcaps(conn, &pcaps, &N_PCAPS);
            {
                int ret = write_PCAP_file(__path, pcaps, N_PCAPS);

                if (!ret) printf("pcap write error\n");
            }
        } else {
            for (size_t i = 0; i < N_PCAPS; i++)
            {
                printf(" id: %ld\n", pcaps[i].id);
                printf("inf: %d\n", pcaps[i].infected);
                printf("nms: %ld\n", pcaps[i].nmessages);
                printf("  q: %ld\n", pcaps[i].q);
                printf(" qr: %ld\n", pcaps[i].qr);
                printf("  r: %ld\n---------------\n", pcaps[i].r);
            }
            
        }
    }

    int debug_pcaps_number = 3;

    PCAPWindowing windowings_all[debug_pcaps_number][N_WSIZE];

    AllWindows all_windows[N_WSIZE];
    memset(all_windows, 0, N_WSIZE * sizeof(AllWindows));
    for (int w = 0; w < N_WSIZE; ++w) {
        all_windows[w].wsize = wsize[w];
        for (int i = 0; i < debug_pcaps_number; ++i) {
            PCAP *pcap = &pcaps[i];
            int n_windows =  N_WINDOWS(pcap->nmessages, wsize[w]);
            all_windows[w].total += n_windows;
            if (pcap->infected) {
                all_windows[w].total_positives += n_windows;
            } else {
                all_windows[w].total_negatives += n_windows;
            }
        }

        all_windows[w].windows = calloc(all_windows[w].total, sizeof(Window*));
        all_windows[w].windows_negatives = calloc(all_windows[w].total_negatives, sizeof(Window*));
        all_windows[w].windows_positives = calloc(all_windows[w].total_positives, sizeof(Window*));
    }

    AllWindowsCursor all_windows_cursor[N_WSIZE];
    memset(all_windows_cursor, 0, N_WSIZE * sizeof(AllWindowsCursor));
    for (int i = 0; i < debug_pcaps_number; ++i) {
        PCAP *pcap = &pcaps[i];

        printf("PCAP %ld having dga=%d and nrows=%ld\n", pcap->id, pcap->infected, pcap->nmessages);

        PCAPWindowing *windowings = windowings_all[i];
        for (int w = 0; w < N_WSIZE; ++w) {
            int nwindows = N_WINDOWS(pcap->nmessages, wsize[w]);

            windowings[w].pcap_id = pcap->id;
            windowings[w].infected = pcap->infected;
            windowings[w].nwindows = nwindows;
            windowings[w].windows = calloc(nwindows, sizeof(Window));
            windowings[w].wsize = wsize[w];

            for (int r = 0; r < nwindows; r++) {
                int nmetrics;
                int pi_i;

                all_windows[w].windows[all_windows_cursor[w].all++] = &windowings[w].windows[r];
                if (pcap->infected) {
                    all_windows[w].windows_positives[all_windows_cursor[w].positives++] = &windowings[w].windows[r];
                } else {
                    all_windows[w].windows_negatives[all_windows_cursor[w].negatives++] = &windowings[w].windows[r];
                }

                windowings[w].windows[r].wsize = wsize[w];
                windowings[w].windows[r].wnum = r;
                windowings[w].windows[r].infected = pcap->infected;
                windowings[w].windows[r].pcap_id = pcap->id;

                nmetrics = N_PIS / N_WSIZE;
                windowings[w].windows[r].nmetrics = nmetrics;
                windowings[w].windows[r].metrics = calloc(nmetrics, sizeof(WindowMetrics));

                pi_i = 0;
                for (int k = 0; k < N_PIS; ++k) {
                    if (pis[k].wsize == wsize[w]) {
                        windowings[w].windows[r].metrics[pi_i].pi = &pis[k];
                        ++pi_i;
                    }
                }
            }
        }

        /*
        
        all_windows: a (_ x 3) vector where 3 is the number of chosen window sizes.
        total_pcaps_windows: a (1 x 3) vector where 3 is the number of chosen window sizes and the element is the number of windows for this size.

        */

        procedure(conn, &pcaps[i], windowings, N_WSIZE);

        {
            char __path[150];
            snprintf(__path, 150, "%s/windows.bin", root_dir);

            FILE *file = fopen(__path, "rb");

            if (!file) {
                perror("write-windows");
                return 0;
            }

            printf("Writing WINDOWS to %s...", __path);

            fwrite(&N_WSIZE, sizeof(int), 1, file);
            fwrite(&debug_pcaps_number, sizeof(int), 1, file);

            for (int i = 0; i < debug_pcaps_number; ++i) {
                PCAP *pcap = &pcaps[i];

                printf("PCAP %ld having dga=%d and nrows=%ld\n", pcap->id, pcap->infected, pcap->nmessages);

                fwrite(&windowings[0].pcap_id, sizeof(int64_t), 1, file);
                fwrite(&windowings[0].infected, sizeof(int), 1, file);

                PCAPWindowing *windowings_sizes = windowings_all[i];
                for (int w = 0; w < N_WSIZE; ++w) {
                    PCAPWindowing* windowings = &windowings_sizes[w];

                    fwrite(&windowings->nwindows, sizeof(int), 1, file);
                    fwrite(&windowings->wsize, sizeof(int), 1, file);

                    for (int r = 0; r < windowings->nwindows; ++r) {
                        Window* window = &windowings->windows[r];
                        fwrite(&window->wnum, sizeof(int), 1, file);
                        fwrite(&window->nmetrics, sizeof(int), 1, file);

                        for (int m = 0; m < window->nmetrics; ++m) {
                            fwrite(&window->metrics[m].dn_bad_05, sizeof(int), 1, file);
                            fwrite(&window->metrics[m].dn_bad_09, sizeof(int), 1, file);
                            fwrite(&window->metrics[m].dn_bad_099, sizeof(int), 1, file);
                            fwrite(&window->metrics[m].dn_bad_0999, sizeof(int), 1, file);
                            fwrite(&window->metrics[m].logit, sizeof(double), 1, file);
                            fwrite(&window->metrics[m].wcount, sizeof(int), 1, file);

                            if (w == 0 && r == 15 && m == 17) {
                                printf("%d\n", window->metrics[m].dn_bad_05);
                                printf("%d\n", window->metrics[m].dn_bad_09);
                                printf("%d\n", window->metrics[m].dn_bad_099);
                                printf("%d\n", window->metrics[m].dn_bad_0999);
                                printf("%f\n", window->metrics[m].logit);
                                printf("%d\n", window->metrics[m].wcount);
                            }
                        }
                    }
                }
            }

            fclose(file);
        }


        {
            PCAPWindowing windowings_pcaps_sizes[debug_pcaps_number][N_WSIZE];
            char __path[150];
            snprintf(__path, 150, "%s/windows.bin", root_dir);

            FILE *file = fopen(__path, "rb");

            if (!file) {
                perror("write-windows");
                return 0;
            }

            fread(&N_WSIZE, sizeof(int), 1, file);
            fread(&debug_pcaps_number, sizeof(int), 1, file);

            printf("           N_WSIZE: %d\n", N_WSIZE);
            printf("debug_pcaps_number: %d\n", debug_pcaps_number);

            for (int i = 0; i < debug_pcaps_number; ++i) {
                PCAP *pcap = &pcaps[i];

                printf("PCAP %ld having dga=%d and nrows=%ld\n", pcap->id, pcap->infected, pcap->nmessages);

                int64_t pcap_id;
                int infected;

                {
                    fread(&pcap_id, sizeof(int64_t), 1, file);
                    fread(&infected, sizeof(int), 1, file);

                    if (pcap_id != pcap->id) printf("ID mismatch %ld != %ld\n", pcap_id, pcap->id);
                    if (infected != pcap->infected) printf("INFECTED mismatch %d != %d\n", infected, pcap->infected);
                }

                PCAPWindowing *windowings_sizes = windowings_pcaps_sizes[i];
                for (int w = 0; w < N_WSIZE; ++w) {

                    PCAPWindowing* windowings = &windowings_sizes[w];

                    fread(&windowings->nwindows, sizeof(int), 1, file);
                    fread(&windowings->wsize, sizeof(int), 1, file);

                    windowings->windows = calloc(windowings->nwindows, sizeof(Window));

                    for (int r = 0; r < windowings->nwindows; ++r) {
                        Window* window = &windowings->windows[r];

                        fread(&window->wnum, sizeof(int), 1, file);
                        fread(&window->nmetrics, sizeof(int), 1, file);

                        window->metrics = calloc(window->nmetrics, sizeof(WindowMetrics));

                        for (int m = 0; m < window->nmetrics; ++m) {
                            fread(&window->metrics[m].dn_bad_05, sizeof(int), 1, file);
                            fread(&window->metrics[m].dn_bad_09, sizeof(int), 1, file);
                            fread(&window->metrics[m].dn_bad_099, sizeof(int), 1, file);
                            fread(&window->metrics[m].dn_bad_0999, sizeof(int), 1, file);
                            fread(&window->metrics[m].logit, sizeof(double), 1, file);
                            fread(&window->metrics[m].wcount, sizeof(int), 1, file);

                            if (w == 0 && r == 15 && m == 17) {
                                printf("%d\n", window->metrics[m].dn_bad_05);
                                printf("%d\n", window->metrics[m].dn_bad_09);
                                printf("%d\n", window->metrics[m].dn_bad_099);
                                printf("%d\n", window->metrics[m].dn_bad_0999);
                                printf("%f\n", window->metrics[m].logit);
                                printf("%d\n", window->metrics[m].wcount);
                            }
                        }
                    }
                }
            }

            fclose(file);
        }

        exit(0);


        // for (int pi_i = 0; pi_i < N_WSIZE; ++pi_i) {
        //     Windowing *windowing = &windowings[pi_i];

        //     for (int k = 0; k < windowing->nwindows; ++k) {
        //         Window *window = &windowing->windows[k];

        //         printf("%d,", windowing->wsize);
        //         printf("%ld,", windowing->pcap_id);
        //         printf("%d,", window->infected);
        //         printf("%d,", window->wnum);
        //         printf("%d,", window->wcount);
        //         printf("[%d][%d]->", pi_i, k);

        //         for (int z = 0; z < window->nmetrics; ++z) {
        //             printf("|%d,%g", z, window->metrics[z].logit);
        //         }

        //         printf("|\n");
        //     }
        // }
    }

    const int N_TH = 5;
    for (int w = 0; w < N_WSIZE; ++w) {
        int tp[N_TH];
        int tn[N_TH];
        int fp[N_TH];
        int fn[N_TH];
        for (int t = 0; t < N_TH; t++) {
            tp[t] = 0;
            tn[t] = 0;
            fp[t] = 0;
            fn[t] = 0;
        }

        for (int i = 0; i < debug_pcaps_number; i++) {
            PCAPWindowing *_w = &windowings_all[i][w];

            char tmp[100];
            sprintf(tmp, "/tmp/pcap_%ld.bin", _w->pcap_id);

            FILE* filep = fopen(tmp, "rb");

            PCAPWindowing pcapw;
            int nmetrics;

            fread(&pcapw.pcap_id, sizeof(int64_t), 1, filep);
            fread(&pcapw.nwindows, sizeof(int), 1, filep);
            fread(&pcapw.wsize, sizeof(int), 1, filep);
            fread(&nmetrics, sizeof(int), 1, filep);

            for (int m = 0; m < nmetrics; ++m) {
                for (int z = 0; z < _w->nwindows; ++z) {
                    fread(&_w->windows[z].metrics[m].wcount, sizeof(int), 1, filep);
                    fread(&_w->windows[z].metrics[m].logit, sizeof(double), 1, filep);
                }
            }

            fclose(filep);
        }

        for (int i = 0; i < debug_pcaps_number; i++) {
            PCAPWindowing *_w = &windowings_all[i][w];

            char tmp[100];
            sprintf(tmp, "/tmp/pcap_%ld.bin", _w->pcap_id);



            FILE* filep = fopen(tmp, "wb");
            

            int nmetrics = _w->windows[0].nmetrics;

            fwrite(&_w->pcap_id, sizeof(int64_t), 1, filep);
            fwrite(&_w->nwindows, sizeof(int), 1, filep);
            fwrite(&_w->wsize, sizeof(int), 1, filep);
            fwrite(&nmetrics, sizeof(int), 1, filep);

            for (int m = 0; m < nmetrics; ++m) {
                for (int z = 0; z < _w->nwindows; ++z) {
                    fwrite(&_w->windows[z].metrics[m].wcount, sizeof(int), 1, filep);
                    fwrite(&_w->windows[z].metrics[m].logit, sizeof(double), 1, filep);
                }
            }

            fclose(filep);

            for (int z = 0; z < _w->nwindows; z++)
            {
                Window *window = &_w->windows[z];
                for (int p = 0; p < window->nmetrics; p++) {
                    double min = window->metrics[p].pi->logit_range.max;
                    double max = window->metrics[p].pi->logit_range.min;
                    double offset = 0.2 * (max - min);
                    double width = 0.2 * (max - min);

                    for (int t = 0; t < N_TH; t++) {
                        double th = min + offset + width * (t * ((double) 1)/N_TH);

                        int pred_inf = window->metrics[p].logit > th;

                        tp[t] += window->infected && pred_inf;
                        fp[t] += !window->infected && pred_inf;
                        fn[t] += window->infected && !pred_inf;
                        tn[t] += !window->infected && !pred_inf;

                    }
                }
                
            }
            
        }
        printf("\n");
        for (int t = 0; t < N_TH; t++) {
            int n = tn[t] + fp[t];
            int p = tp[t] + fn[t];
            printf("%4.2f\t%4.2f\t%4.2f\t%4.2f\t%4.2f\n", (t * ((double)1)/N_TH), ((double)tn[t]) / n, ((double)fp[t]) / n, ((double)fn[t]) / p, ((double)tp[t]) / p);
        }
        printf("\n\n");
    }


    // all_windows
    // total_pcaps_windows

    double split_percentage = 0.7;

    for (int w = 0; w < N_WSIZE; ++w) {
        AllWindows* aw = &all_windows[w];

        int n = floor(aw->total_negatives * split_percentage);

        Window* extracted[n];

        extract(aw->windows_negatives, n, extracted);

        for (int i = 0; i < n; ++i) {
            printf("%d\n", extracted[i]->pcap_id);
            printf("%d\n", extracted[i]->infected);
            printf("%f\n\n", extracted[i]->metrics->logit);
        }
    }

    for (int w = 0; w < N_WSIZE; ++w) {
        WindowingDataset wd;
        double tot_n = all_windows[w].total_negatives;
        double tot_p = all_windows[w].total_positives;

        wd.train.n.total = floor(tot_n * split_percentage);
        wd.train.n.windows = calloc(wd.train.n.total, sizeof(Window*));

        wd.test.n.total = tot_n - wd.train.n.total;
        wd.test.n.windows = calloc(wd.test.n.total, sizeof(Window*));

        wd.train.p.total = floor(tot_p * split_percentage);
        wd.train.p.windows = calloc(wd.train.p.total, sizeof(Window*));

        wd.test.p.total = tot_p - wd.train.p.total;
        wd.test.p.windows = calloc(wd.test.p.total, sizeof(Window*));

        printf("%20s\t%d\n", "total_negatives", (int) tot_n);
        printf("%20s\t%d\n", "total_positives", (int) tot_p);
        printf("%20s\t%d\n", "wd.train.n.total", wd.train.n.total);
        printf("%20s\t%d\n", "wd.test.n.total", wd.test.n.total);
        printf("%20s\t%d\n", "wd.train.p.total", wd.train.p.total);
        printf("%20s\t%d\n", "wd.test.p.total", wd.test.p.total);

        extract_wd(all_windows[w].windows_negatives, &wd.train.n, &wd.test.n);
        extract_wd(all_windows[w].windows_positives, &wd.train.p, &wd.test.p);
    }

    // printf("%5s,%5s,%8d", "", "", overrall.nwindows);
    // printf("%8d,%8d,%8d,%8d,", overrall.nw_infected_05, overrall.nw_infected_09, overrall.nw_infected_099, overrall.nw_infected_0999);
    // printf("%8d\n", overrall.correct_predictions);

    return EXIT_SUCCESS;
}