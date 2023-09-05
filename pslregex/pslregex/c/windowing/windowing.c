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

void CDPGquery(PGconn* conn, PCAP* pcap, int window_size, int th_top10m, Overrall* overrall)
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
    


    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        int nrows = PQntuples(res);
        Message* messages = calloc(nrows, sizeof(Message));

        int nwindows = nrows / window_size + (nrows % window_size > 0);
        // printf("%d\t%d\t%d\t%d\n", nrows, window_size, nrows / window_size, nwindows);


        Window* windows = calloc(nwindows, sizeof(Window));

        for(int r = 0; r < nwindows; r++) {
            windows[r].wnum = r;
            windows[r].pcap_id = pcap->id;
            windows[r].infected = pcap->infected;
        }

        int wnum = -1;
        Window* cursor = &windows[0];
        for(int r = 0; r < nrows; r++) {
            if (r != 0 && r % window_size == 0) {
                cursor = &windows[++wnum];
            }
            parse_message(res, r, &messages[r]);

            cursor->logit += messages[r].logit;

            double value = messages[r].value;
            if (messages[r].top10m < th_top10m) {
                value = 0;
            }

            cursor->dn_bad_05 += value >= 0.5;
            cursor->dn_bad_09 += value >= 0.9;
            cursor->dn_bad_099 += value >= 0.99;
            cursor->dn_bad_0999 += value >= 0.999;
            cursor->wsize += 1;
        }

        int infected_05 = 0;
        int infected_09 = 0;
        int infected_099 = 0;
        int infected_0999 = 0;
        for (int i = 0; i < nwindows; i++) {
            Window* window = &windows[i];
            infected_05 += window->dn_bad_05 > 0;
            infected_09 += window->dn_bad_09 > 0;
            infected_099 += window->dn_bad_099 > 0;
            infected_0999 += window->dn_bad_0999 > 0;
        }

        int correct_predictions;
        if (pcap->infected) {
            correct_predictions = (infected_05 > 0) + (infected_09 > 0) + (infected_099 > 0) + (infected_0999 > 0);
        } else {
            correct_predictions = (infected_05 == 0) + (infected_09 == 0) + (infected_099 == 0) + (infected_0999 == 0);
        }

        overrall->nwindows += nwindows;
        overrall->nw_infected_05 += infected_05;
        overrall->nw_infected_09 += infected_09;
        overrall->nw_infected_099 += infected_099;
        overrall->nw_infected_0999 += infected_0999;
        overrall->correct_predictions += correct_predictions;
        

        printf("%5ld\t%5d\t%8d", pcap->id, pcap->infected, nwindows);
        printf("%8d\t%8d\t%8d\t%8d\t", infected_05, infected_09, infected_099, infected_0999);
        printf("%8d\n", correct_predictions);

        // for(int r = 0; r < nrows; r++) {
        //     // printf("%10ld\t%10.4f\t%10.4f\t%d\n", messages[r].fn_req, messages[r].value, messages[r].logit, messages[r].is_response);
        // }
    }

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

void get_pcaps(PGconn* conn, PCAP* pcaps) {
    PGresult* res = PQexec(conn, "SELECT id, (not malware_id = 28) as infected, qr, q, r FROM pcap");

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        int nrows = PQntuples(res);

        for(int r = 0; r < nrows; r++)
        {
            pcaps[r].id = atoi(PQgetvalue(res, r, 0));
            pcaps[r].infected = PQgetvalue(res, r, 1)[0] == 't';
            pcaps[r].qr = atoi(PQgetvalue(res, r, 2));
            pcaps[r].q = atoi(PQgetvalue(res, r, 3));
            pcaps[r].r = atoi(PQgetvalue(res, r, 4));
        }
    }
}

int
windowing ()
{
    PGconn* conn = CDPGget_connection();

    int64_t pcaps_number = get_pcaps_number(conn);

    PCAP* pcaps = calloc(pcaps_number, sizeof(PCAP));

    get_pcaps(conn, pcaps);

    int wss[5] = { 1, 5, 25, 50, 250 };

    int th_top10m = 0;

    for (int k = 0; k < 5; ++k) {
        int ws = wss[k];

        printf("window size: %d\n", ws);

        Overrall overrall;
        memset(&overrall, 0, sizeof(Overrall));

        for (int i = 0; i < pcaps_number; ++i) {
            if (pcaps[i].infected) continue;
            CDPGquery(conn, &pcaps[i], ws, th_top10m, &overrall);
        }

        printf("%5s\t%5s\t%8d", "", "", overrall.nwindows);
        printf("%8d\t%8d\t%8d\t%8d\t", overrall.nw_infected_05, overrall.nw_infected_09, overrall.nw_infected_099, overrall.nw_infected_0999);
        printf("%8d\n", overrall.correct_predictions);
    }

    return EXIT_SUCCESS;
}