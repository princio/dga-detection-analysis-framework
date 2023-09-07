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
void procedure(char* root_dir, PGconn* conn, PCAP* pcap, PCAPWindowing pcapwindowings_byWSIZE[], int N_WSIZE)
{
    int ret;
    PGresult* pgresult;
    int nrows;

    ret = read_PCAPWindowings(root_dir, pcap, pcapwindowings_byWSIZE, N_WSIZE);

    if (ret == 1) {
        return;
    }
    
    {
        char sql[1000];
        int pgresult_binary = 1;

        sprintf(sql,
                "SELECT FN_REQ, VALUE, LOGIT, IS_RESPONSE, TOP10M, DYNDNS FROM MESSAGES_%ld AS M "
                "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                "JOIN DN AS DN ON M.DN_ID=DN.ID "
                "ORDER BY FN_REQ",
                pcap->id);

        pgresult = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, !pgresult_binary);
    }
    
    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK) {
        printf("No data\n");
        return;
    }

    nrows = PQntuples(pgresult);

    if (nrows != pcap->nmessages) {
        printf("Errorrrrrr:\t%d\t%ld\n", nrows, pcap->nmessages);
    }

    int wnum_max[N_WSIZE];
    memset(wnum_max, 0, sizeof(int) * N_WSIZE);
    for(int r = 0; r < nrows; r++) {
        Message message;

        parse_message(pgresult, r, &message);

        for (int w0 = 0; w0 < N_WSIZE; ++w0) {
            PCAPWindowing *pcapwindowing;
            int wnum;
            Window *window;
            
            pcapwindowing = &pcapwindowings_byWSIZE[w0];

            wnum = (int) floor(message.fn_req / pcapwindowing->wsize);

            if (wnum >= pcapwindowing->nwindows) {
                printf("ERROR\n");
                printf("      wnum: %d\n", wnum);
                printf("     wsize: %d\n", pcapwindowing->wsize);
                printf(" fnreq_max: %ld\n", pcap->fnreq_max);
                printf("  nwindows: %d\n", pcapwindowing->nwindows);
            }


            wnum_max[w0] = wnum_max[w0] < wnum ? wnum : wnum_max[w0];

            window = &pcapwindowing->windows[wnum];

            for (int w2 = 0; w2 < window->nmetrics; w2++) {
                int whitelistened = 0;
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
                    whitelistened = 1;
                }
                if (logit == INFINITY) {
                    logit = pi->infinite_values.pinf;
                } else
                    if (logit == (-1 * INFINITY)) {
                    logit = pi->infinite_values.ninf;
                }

                ++metrics->wcount;
                metrics->logit += logit;
                metrics->whitelistened += whitelistened;
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
    }

    for (int w0 = 0; w0 < N_WSIZE; ++w0) {
        printf("wnum_max: %d / %d -- %ld / %ld\n", wnum_max[w0], pcapwindowings_byWSIZE[w0].wsize, pcap->fnreq_max, N_WINDOWS(pcap->fnreq_max, pcapwindowings_byWSIZE[w0].wsize));
    }


    PQclear(pgresult);

    ret = write_PCAPWindowings(root_dir, pcap, pcapwindowings_byWSIZE, N_WSIZE);
    if(!ret) {
        printf("Writing PCAPWindowing for %d error\n", pcap->id);
    }
}

int has_been_extracted(int n, int array[n], int dado) {
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
    int total = wd3_train->number + wd3_test->number;
    int extracted[total]; // 1 means test, 0 means train
    int i;

    memset(extracted, 0, sizeof(int) * total);

    srand(time(NULL));


    WindowingDataset3* wd3_toextract = wd3_train->number > wd3_test->number ? wd3_train : wd3_test;
    WindowingDataset3* wd3_tofill = wd3_train->number > wd3_test->number ? wd3_test : wd3_train;

    int guard = total * 100;
    int guard_counter = 0;
    i = 0;
    while (++guard_counter < guard && i < wd3_toextract->number) {
        int dado = rand() % total;

        if (extracted[dado]) continue;

        extracted[dado] = 1;
        wd3_toextract->windows[i++] = windows[dado];
        if (wd3_toextract->number < i) {
            printf("Error wd3_toextract->number <= i (%d <= %d)\n", wd3_toextract->number , i);
        }
    }
    if (i != wd3_toextract->number) {
        printf("Error %d != %d\n", i, wd3_toextract->number);
    }

    if (guard_counter >= guard) {
        printf("Guard error\n");
    }

    i = 0;
    for (int j = 0; j < total; ++j) {
        if (0 == extracted[j]) {
            wd3_tofill->windows[i++] = windows[j];
            if (wd3_tofill->number < i) {
                printf("Error wd3_tofill->number <= i (%d <= %d)\n", wd3_tofill->number , i);
            }
        }
    }
    if (i != wd3_tofill->number) {
        printf("Error %d != %d\n", i, wd3_tofill->number);
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

void get_pcaps(char* root_dir, PGconn* conn, PCAP** pcaps_ptr, int* N_PCAPS_ptr) {
    int ret;
    int N_PCAPS;
    PCAP* pcaps;
    PGresult* pgresult;

    char path[150];
    snprintf(path, 150, "%s/pcaps.bin", root_dir);

    ret = read_PCAP_file(path, pcaps_ptr, N_PCAPS_ptr);

    if (ret == 1) {
        return;
    }


    N_PCAPS = get_pcaps_number(conn);

    pcaps = calloc(N_PCAPS, sizeof(PCAP));

    pgresult = PQexec(conn, "SELECT pcap.id, mw.dga as dga, qr, q, r FROM pcap JOIN malware as mw ON malware_id = mw.id");

    if (PQresultStatus(pgresult) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        int nrows = PQntuples(pgresult);

        for(int r = 0; r < nrows; r++)
        {
            pcaps[r].id = atoi(PQgetvalue(pgresult, r, 0));
            pcaps[r].infected = atoi(PQgetvalue(pgresult, r, 1));
            pcaps[r].qr = atoi(PQgetvalue(pgresult, r, 2));
            pcaps[r].q = atoi(PQgetvalue(pgresult, r, 3));
            pcaps[r].r = atoi(PQgetvalue(pgresult, r, 4));

            {
                int nmessages = 0;
                char sql[1000];
                // sprintf(sql,
                //     "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
                //     "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                //     "JOIN DN AS DN ON M.DN_ID=DN.ID",
                //     pcaps[r].id);
                sprintf(sql,
                    "SELECT COUNT(*) FROM MESSAGES_%ld",
                    pcaps[r].id);

                PGresult* res_nrows = PQexec(conn, sql);
                if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                    printf("No data\n");
                } else {
                    nmessages = atoi(PQgetvalue(res_nrows, 0, 0));
                }
                PQclear(res_nrows);
                pcaps[r].nmessages = nmessages;
            }

            {
                int fnreq_max = 0;
                char sql[1000];
                // sprintf(sql,
                //     "SELECT COUNT(*) FROM MESSAGES_%ld AS M "
                //     "JOIN (SELECT * FROM DN_NN WHERE NN_ID=7) AS DN_NN ON M.DN_ID=DN_NN.DN_ID "
                //     "JOIN DN AS DN ON M.DN_ID=DN.ID",
                //     pcaps[r].id);
                sprintf(sql,
                    "SELECT MAX(FN_REQ) FROM MESSAGES_%ld",
                    pcaps[r].id);

                PGresult* res_nrows = PQexec(conn, sql);
                if (PQresultStatus(res_nrows) != PGRES_TUPLES_OK) {
                    printf("No data\n");
                } else {
                    fnreq_max = atoi(PQgetvalue(res_nrows, 0, 0));
                }
                PQclear(res_nrows);
                pcaps[r].fnreq_max = fnreq_max;
            }
        }
    }

    PQclear(pgresult);

    *pcaps_ptr = pcaps;
    *N_PCAPS_ptr = N_PCAPS;


    ret = write_PCAP_file(path, pcaps, (const int) N_PCAPS);
    if (!ret) printf("pcap write error\n");
}

void print_pi(Pi *pi, int z) {
    printf("%d\n", z);
    printf(" Window size:  %d\n", pi->wsize);
    printf("Whitelisting:  (%d, %f)\n", pi->whitelisting.rank, pi->whitelisting.value);
    printf("          NN:  %s\n", NN_NAMES[pi->nn]);
    printf("   Windowing:  %s\n\n", WINDOWING_NAMES[pi->windowing]);
}

void generate_parameters(char* root_dir, Pi** pis_ptr, int* N_PIS_ptr, int wsize[], int N_WSIZE) {
    int N_PIS;
    Pi* pis;

    char __path[150];
    snprintf(__path, 150, "%s/parameters.bin", root_dir);

    {
        int ret = read_Pi_file(__path, pis_ptr, N_PIS_ptr);

        if (ret == 1) {
            return;
        }
    }

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

    N_PIS = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i3 = 0; i3 < count_ws; ++i3) {
                    for (size_t i4 = 0; i4 < count_iv; ++i4) {
                        ++N_PIS;
                    }
                }
            }
        }
    }

    printf("Number of parameters: %d\n", N_PIS);

    pis = calloc(N_PIS, sizeof(Pi));

    *pis_ptr = pis;
    *N_PIS_ptr = N_PIS;

    N_PIS = 0;
    for (size_t i0 = 0; i0 < count_nn; ++i0) {
        for (size_t i1 = 0; i1 < count_wl; ++i1) {
            for (size_t i2 = 0; i2 < count_wt; ++i2) {
                for (size_t i3 = 0; i3 < count_ws; ++i3) {
                    for (size_t i4 = 0; i4 < count_iv; ++i4) {
                        pis[N_PIS].infinite_values = infinitevalues[i4];
                        pis[N_PIS].nn = nn[i0];
                        pis[N_PIS].whitelisting = whitelisting[i1];
                        pis[N_PIS].windowing = windowing_types[i2];
                        pis[N_PIS].wsize = wsize[i3];
                        pis[N_PIS].id = N_PIS;

                        ++N_PIS;
                    }
                }
            }
        }
    }

    {
        int ret = write_Pi_file(__path, *pis_ptr, *N_PIS_ptr);

        if (!ret) printf("Pi-s write error\n");
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

    PGconn* conn;

    int N_PIS;
    Pi* pis;

    int N_PCAPS;
    PCAP* pcaps;

    int N_WSIZE = 3;
    int wsize[] = { 100, 500, 2500 };

    int debug_pcaps_number;

    conn = CDPGget_connection();

    generate_parameters(root_dir, &pis, &N_PIS, wsize, N_WSIZE);
    
    get_pcaps(root_dir, conn, &pcaps, &N_PCAPS);

    /**
     *  ONLY FOR DEBUG
     */
    debug_pcaps_number = N_PCAPS;
    /**
     *  
     */

    // qui abbiamo tutti i PCAP e tutti i parameter-set



    const int N_METRICS = N_PIS / N_WSIZE; 
    printf("    N_PIS\t%d\n", N_PIS);
    printf("N_METRICS\t%d\n", N_METRICS);
    

    /**
     * @brief Qui inizializzaziom, per ogni wsize, uno struct AllWindows il quale detiene un array di puntatori a tutte le finestr
     * 
     */
    AllWindows all_windows[N_WSIZE];
    memset(all_windows, 0, N_WSIZE * sizeof(AllWindows));
    for (int w = 0; w < N_WSIZE; ++w) {
        all_windows[w].wsize = wsize[w];
        for (int i = 0; i < debug_pcaps_number; ++i) {
            PCAP *pcap = &pcaps[i];
            int n_windows =  N_WINDOWS(pcap->fnreq_max, wsize[w]);

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


    PCAPWindowing pcapwindowings_byPcap_byWSIZE[debug_pcaps_number][N_WSIZE];
    AllWindowsCursor all_windows_cursor[N_WSIZE];
    memset(all_windows_cursor, 0, N_WSIZE * sizeof(AllWindowsCursor));
    for (int i = 0; i < debug_pcaps_number; ++i) {
        PCAP *pcap = &pcaps[i];

        printf("PCAP %ld having dga=%d and nrows=%ld\n", pcap->id, pcap->infected, pcap->nmessages);

        PCAPWindowing *pcapwindowings_byWSIZE = pcapwindowings_byPcap_byWSIZE[i];
        for (int w = 0; w < N_WSIZE; ++w) {
            int nwindows = N_WINDOWS(pcap->fnreq_max, wsize[w]);

            PCAPWindowing *pcapwindowing = &pcapwindowings_byWSIZE[w];

            pcapwindowing->pcap_id = pcap->id;
            pcapwindowing->infected = pcap->infected;
            pcapwindowing->nwindows = nwindows;
            pcapwindowing->windows = calloc(nwindows, sizeof(Window));
            pcapwindowing->wsize = wsize[w];

            for (int r = 0; r < nwindows; r++) {
                Window* window = &pcapwindowing->windows[r];
                int pi_i;

                all_windows[w].windows[all_windows_cursor[w].all++] = window;
                if (pcap->infected) {
                    all_windows[w].windows_positives[all_windows_cursor[w].positives++] = window;
                } else {
                    all_windows[w].windows_negatives[all_windows_cursor[w].negatives++] = window;
                }

                window->wsize = wsize[w];
                
                window->wnum = r;
                window->infected = pcap->infected;
                window->pcap_id = pcap->id;
                window->nmetrics = N_METRICS;
                window->metrics = calloc(N_METRICS, sizeof(WindowMetrics));

                pi_i = 0;
                for (int k = 0; k < N_PIS; ++k) {
                    if (pis[k].wsize == wsize[w]) {
                        window->metrics[pi_i].pi = &pis[k];
                        ++pi_i;
                    }
                }
            }
        }

        /*
        
        all_windows: a (_ x 3) vector where 3 is the number of chosen window sizes.
        total_pcaps_windows: a (1 x 3) vector where 3 is the number of chosen window sizes and the element is the number of windows for this size.

        */

        procedure(root_dir, conn, pcap, pcapwindowings_byWSIZE, N_WSIZE);
    }

    {
        FILE*fp = fopen("pcap.csv", "w");
        fprintf(fp, "wnum,pcap\n");
        for (size_t p = 0; p < N_PCAPS; p++)
        {
            PCAPWindowing* pcapwindowing = &pcapwindowings_byPcap_byWSIZE[p][2];
            for (size_t i = 0; i < pcapwindowing->nwindows; i++)
            {
                fprintf(fp, "%d,%d\n", pcapwindowing->windows[i].wnum, pcapwindowing->windows[i].pcap_id);
            }
        }
        fclose(fp);
    }

    {
        FILE*fp = fopen("allwindows.csv", "w");
        fprintf(fp, "wnum,pcap\n");
        AllWindows* all_windows_2500 = &all_windows[2];
        for (size_t i = 0; i < all_windows_2500->total_positives; i++)
        {
            fprintf(fp, "%d,%d\n", all_windows_2500->windows_positives[i]->wnum, all_windows_2500->windows_positives[i]->pcap_id);
        }
        fclose(fp);
    }

    // const int N_TH = 5;
    // for (int w = 0; w < N_WSIZE; ++w) {
    //     int tp[N_TH];
    //     int tn[N_TH];
    //     int fp[N_TH];
    //     int fn[N_TH];
    //     for (int t = 0; t < N_TH; t++) {
    //         tp[t] = 0;
    //         tn[t] = 0;
    //         fp[t] = 0;
    //         fn[t] = 0;
    //     }

    //     for (int i = 0; i < debug_pcaps_number; i++) {
    //         PCAPWindowing *pcapwindowing = &pcapwindowings_byPcap_byWSIZE[i][w];

    //         int nmetrics = pcapwindowing->windows[0].nmetrics;

    //         for (int z = 0; z < pcapwindowing->nwindows; z++)
    //         {
    //             Window *window = &pcapwindowing->windows[z];
    //             for (int p = 0; p < window->nmetrics; p++) {
    //                 double min = window->metrics[p].pi->logit_range.max;
    //                 double max = window->metrics[p].pi->logit_range.min;
    //                 double offset = 0.2 * (max - min);
    //                 double width = 0.2 * (max - min);

    //                 for (int t = 0; t < N_TH; t++) {
    //                     double th = min + offset + width * (t * ((double) 1)/N_TH);

    //                     int pred_inf = window->metrics[p].logit > th;

    //                     tp[t] += window->infected && pred_inf;
    //                     fp[t] += !window->infected && pred_inf;
    //                     fn[t] += window->infected && !pred_inf;
    //                     tn[t] += !window->infected && !pred_inf;

    //                 }
    //             }
    //         }
    //     }
    //     printf("\n");
    //     for (int t = 0; t < N_TH; t++) {
    //         int n = tn[t] + fp[t];
    //         int p = tp[t] + fn[t];
    //         printf("%4.2f\t%4.2f\t%4.2f\t%4.2f\t%4.2f\n", (t * ((double)1)/N_TH), ((double)tn[t]) / n, ((double)fp[t]) / n, ((double)fn[t]) / p, ((double)tp[t]) / p);
    //     }
    //     printf("\n\n");
    // }


    // all_windows
    // total_pcaps_windows

    double split_percentage = 0.1;

    // for (int w = 0; w < N_WSIZE; ++w) {
    //     AllWindows* aw = &all_windows[w];

    //     int n = floor(aw->total_negatives * split_percentage);

    //     Window* extracted[n];

    //     extract(aw->windows_negatives, n, extracted);

    //     for (int i = 0; i < n; ++i) {
    //         printf("%5d\t%d\t%f\n", extracted[i]->pcap_id, extracted[i]->infected, extracted[i]->metrics->logit);
    //     }
    // }




    for (int w = 2; w < N_WSIZE; ++w) {
        printf("\n ---- WSIZE = %d ----\n\n", wsize[w]);

        FILE* __fp = fopen("/tmp/fp.csv", "w");
        for (size_t i = 0; i < all_windows[w].total_negatives; i++)
        {
            fprintf(__fp, "%d,%d\n", all_windows[w].windows_negatives[i]->wnum, all_windows[w].windows_negatives[i]->pcap_id);
        }
        fclose(__fp);

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

        printf("total\n");
        printf("  n:\t%d\n", tot_n);
        printf("  p:\t%d\n", tot_p);
        printf("train\n");
        printf("  n:\t%d\n", wd.train.n.number);
        printf("  p:\t%d\n", wd.train.p.number);
        printf("test\n");
        printf("  n:\t%d\n", wd.test.n.number);
        printf("  p:\t%d\n", wd.test.p.number);

        extract_wd(all_windows[w].windows_negatives, &wd.train.n, &wd.test.n);
        extract_wd(all_windows[w].windows_positives, &wd.train.p, &wd.test.p);

        // for (size_t i = 0; i < wd.test.p.number; i++)
        // {
        //     printf("%d\t", i);
        //     if (wd.test.p.windows[i]) {
        //         printf("%d\t%d\n", wd.test.p.windows[i]->wnum, wd.test.p.windows[i]->pcap_id);
        //     } else {
        //         printf("null\n");
        //     }
        // }
        


        printf("\n ---- ---- ----\n\n");


        int nmetrics = wd.train.n.windows[0]->nmetrics;
        double logit_max[nmetrics];
        for (size_t m = 0; m < nmetrics; m++) logit_max[m] = - DBL_MAX;

        for (size_t i = 0; i < wd.train.n.number; i++)
        {
            Window* window = wd.train.n.windows[i];

            for (size_t m = 0; m < nmetrics; m++)
            {
                double _l = window->metrics[m].logit;
                if (logit_max[m] < _l) logit_max[m] = _l;
            }
        }

        // for (size_t m = 0; m < nmetrics; m++)
        // {
        //     printf("%f\n", logit_max[m]);
        // }

        int tn[nmetrics];
        int fp[nmetrics];
        memset(tn, 0, sizeof(int) * nmetrics);
        memset(fp, 0, sizeof(int) * nmetrics);
        for (size_t i = 0; i < wd.test.n.number; i++)
        {
            Window* window = wd.test.n.windows[i];

            for (size_t m = 0; m < nmetrics; m++)
            {
                if (window->metrics[m].logit >= logit_max[m]) ++fp[m];
                else ++tn[m];
            }
        }

        int tp[nmetrics];
        int fn[nmetrics];
        memset(tp, 0, sizeof(int) * nmetrics);
        memset(fn, 0, sizeof(int) * nmetrics);
        for (size_t i = 0; i < wd.test.p.number; i++)
        {
            Window* window = wd.test.p.windows[i];

            for (size_t m = 0; m < nmetrics; m++)
            {
                if (window->metrics[m].logit < logit_max[m]) ++fn[m];
                else ++tp[m];
            }
        }

        int avg[4];
        for (size_t m = 0; m < nmetrics; m++) {
            printf("%d %d %d %d\n", tn[m], fp[m], fn[m], tp[m]);
            avg[0] += tn[m];
            avg[1] += fp[m];
            avg[2] += fn[m];
            avg[3] += tp[m];
        }

        printf("\n%f %f %f %f\n", ((double)avg[0]) / nmetrics, ((double)avg[1]) / nmetrics, ((double)avg[2]) / nmetrics, ((double)avg[3]) / nmetrics);

        break;
    }
    

    // printf("%5s,%5s,%8d", "", "", overrall.nwindows);
    // printf("%8d,%8d,%8d,%8d,", overrall.nw_infected_05, overrall.nw_infected_09, overrall.nw_infected_099, overrall.nw_infected_0999);
    // printf("%8d\n", overrall.correct_predictions);

    return EXIT_SUCCESS;
}