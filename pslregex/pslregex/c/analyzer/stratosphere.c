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

#include "../trie.h"

#include <libpq-fe.h>
#include <15/server/catalog/pg_type_d.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* for ntohl/htonl */
#include <netinet/in.h>
#include <arpa/inet.h>


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


int get_dns_number(PGconn* conn) {
    PGresult* res = PQexec(conn, "SELECT count(*) FROM dn");
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

void get_dns_labels(PGconn* conn, TrieNode* root) {
    PGresult* res = PQexec(conn, "SELECT dn FROM dn");

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("No data\n");
    }
    else
    {
        int nrows = PQntuples(res);

        for(int r = 0; r < nrows; r++)
        {
            char *domain = PQgetvalue(res, r, 0);

            int nlabels = 0;
            char *cursor = domain;
            do {
                ++nlabels;
                cursor = strchr(domain, '.');
            } while (cursor);
        }
    }
}

int
windowing ()
{
    PGconn* conn;
    struct TrieNode *root;

    conn = CDPGget_connection();
    root = trie_load("iframe.csv");

    

    int64_t pcaps_number = get_pcaps_number(conn);

    PCAP* pcaps = calloc(pcaps_number, sizeof(PCAP));

    get_dns(conn);

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