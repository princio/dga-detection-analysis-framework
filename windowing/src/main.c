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

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* for ntohl/htonl */
#include <netinet/in.h>
#include <arpa/inet.h>

#define N_WINDOWS(A,B) (A / B + (A % B > 0))

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

void get_pcaps(PGconn* conn, PCAP* pcaps) {
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
            
            break;
        }
    }

    PQclear(res);
}

void procedure(PGconn* conn, PCAP* pcap, Windowing windowings[], int N_WINDOWING)
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
            Windowing *windowing;
            int wnum;
            Window *window;
            
            windowing = &windowings[w0];

            wnum = (int) floor(message.fn_req / windowing->wsize);

            window = &windowing->windows[wnum];

            ++window->wcount;

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

                metrics->logit += logit;
                metrics->dn_bad_05 += value >= 0.5;
                metrics->dn_bad_09 += value >= 0.9;
                metrics->dn_bad_099 += value >= 0.99;
                metrics->dn_bad_0999 += value >= 0.999;
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

    getchar();
    PQclear(res);
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

void print_pi(Pi *pi, int z) {
    printf("%d\n", z);
    printf(" Window size:  %d\n", pi->wsize);
    printf("Whitelisting:  (%d, %f)\n", pi->whitelisting.rank, pi->whitelisting.value);
    printf("          NN:  %s\n", NN_NAMES[pi->nn]);
    printf("   Windowing:  %s\n\n", WINDOWING_NAMES[pi->windowing]);
}

int
main (int argc, char* argv[])
{
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

    InfiniteValues infinitevalues = { .ninf = -20, .pinf = 20 };

    int wsize[] = { 100, 500, 2500 };

    const int N_WSIZE = 3;

    int n = 0;
    for (int i0 = 0; i0 < 4; ++i0) {
        for (int i1 = 0; i1 < 5; ++i1) {
            for (int i2 = 0; i2 < 3; ++i2) {
                for (int i3 = 0; i3 < 3; ++i3) {
                    ++n;
                }
            }
        }
    }

    printf("%d\n", n);

    const int N_PIS = n;
    Pi pis[N_PIS];

    n = 0;
    for (int i0 = 0; i0 < 4; ++i0) {
        for (int i1 = 0; i1 < 5; ++i1) {
            for (int i2 = 0; i2 < 3; ++i2) {
                for (int i3 = 0; i3 < 3; ++i3) {
                    pis[n].infinite_values = infinitevalues;
                    pis[n].nn = nn[i0];
                    pis[n].whitelisting = whitelisting[i1];
                    pis[n].windowing = windowing_types[i2];
                    pis[n].wsize = wsize[i3];
                    ++n;
                }
            }
        }
    }

    PGconn* conn = CDPGget_connection();

    int64_t pcaps_number = get_pcaps_number(conn);

    PCAP* pcaps = calloc(pcaps_number, sizeof(PCAP));

    // Overrall overralls[pcaps_number][N_PIS];

    get_pcaps(conn, pcaps);

    for (int i = 0; i < pcaps_number; ++i) {
    
        Windowing windowings[N_WSIZE];

        PCAP *pcap = &pcaps[i];

        printf("PCAP %ld having dga=%d and nrows=%ld\n", pcap->id, pcap->infected, pcap->nmessages);


        for (int w = 0; w < N_WSIZE; ++w) {
            int nwindows = N_WINDOWS(pcap->nmessages, wsize[w]);

            windowings[w].pcap_id = pcap->id;
            windowings[w].infected = pcap->infected;
            windowings[w].nwindows = nwindows;
            windowings[w].windows = calloc(nwindows, sizeof(Window));
            windowings[w].wsize = wsize[w];

            for(int r = 0; r < nwindows; r++) {
                int nmetrics;
                int pi_i;
                windowings[w].windows[r].wnum = r;
                windowings[w].windows[r].infected = pcap->infected;

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

        procedure(conn, &pcaps[i], windowings, N_WSIZE);

        for (int pi_i = 0; pi_i < N_WSIZE; ++pi_i) {
            Windowing *windowing = &windowings[pi_i];

            for (int k = 0; k < windowing->nwindows; ++k) {
                Window *window = &windowing->windows[k];

                printf("%d,", windowing->wsize);
                printf("%ld,", windowing->pcap_id);
                printf("%d,", window->infected);
                printf("%d,", window->wnum);
                printf("%d,", window->wcount);
                printf("[%d][%d]->", pi_i, k);

                for (int z = 0; z < window->nmetrics; ++z) {
                    printf("|%d,%g", z, window->metrics[z].logit);
                }

                printf("|\n");
            }
        }

        break;
    }

    // printf("%5s,%5s,%8d", "", "", overrall.nwindows);
    // printf("%8d,%8d,%8d,%8d,", overrall.nw_infected_05, overrall.nw_infected_09, overrall.nw_infected_099, overrall.nw_infected_0999);
    // printf("%8d\n", overrall.correct_predictions);

    return EXIT_SUCCESS;
}