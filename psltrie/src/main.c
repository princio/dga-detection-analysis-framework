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

#include "psltrie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct PSLTTestFile {
    size_t index;
    PSLTDomainSuffix suffix;
    PSLTSuffixGroup group;
    PSLTDomain tests[3];
};

char* unused;


void _print_test_domain(PSLT* pslt, PSLTDomain domain) {
    PSLTObject object;

    strcpy(object.domain, domain);

    pslt_domain_run(pslt, &object);

    printf("%s\n", domain);
    PSLT_FOR_GROUP(g) {
        printf("\t%10s\t%-20s\t%-10s\t%s\n", PSLT_GROUP_STR[g], object.suffixes[g] ? object.suffixes[g]->suffix->suffix : "-", "", object.suffixless[g]);
    }
}

void print_help(void) {
    printf(
    "Usage: dns_parse PSLLIST_PATH [print|test|csv|csvtest] [OPTIONS]\n\n"
    "- print\n"
    "  Print the psl trie in a tree format.\n"
    "\n"
    "- test DOMAINS\n"
    "  Perform psltrie to each domain belonging to DOMAINS, whitespace separeted.\n"
    "\n"
    "- csv COL_NUMBER CSV_IN CSV_OUT\n"
    "  Perform psltrie to each domain belonging to CSV_IN. The COL_NUMBER option indicates\n"
    "  the column where domains are stored.\n"
    "  The result will be saved in CSV_OUT in the following format: .\n"
    "  -> dn,bdn,tld,icann,private,dn_tld,dn_icann,dn_private.\n"
    );
}

int _parse_int(const char *str, int base, int *value) {
    char *endptr = 0;
    
    errno = 0;

    long val = strtol(str, &endptr, base);

    *value = (int) val;

    return !(endptr != str && errno == 0 && endptr && *endptr == '\0');
}

int main (int argc, char* argv[]) {
    setbuf(stdout, NULL);

    PSLT* pslt;

    pslt_logger_file = fopen("/tmp/psltrie.log", "w");

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        print_help();
        exit(1);
    }

    if (argc > 1) {
        if (!strcmp(argv[1], "help")) {
            print_help();
            exit(0);
        }
        pslt = pslt_trie_load(argv[1]);
        if (pslt == NULL) {
            fprintf(stderr, "Impossible to load PSLT.\n");
            exit(1);
        }
    }

    char* command = argv[2];

    if (!strcmp(command, "print")) {
        pslt_trie_print(pslt);
    } else
    if (!strcmp(command, "csv")) {
        int dn_col;

        if (_parse_int(argv[3], 10, &dn_col)) {
            fprintf(stderr, "Impossible to parse the column number.\n");
            exit(1);
        }

        pslt_csv(pslt, dn_col, argv[4], argv[5]);
    }
    else
    if (!strcmp(command, "test")) {
        PSLTDomain domain;
        for (int i = 3; i < argc; i++) {
            strcpy(domain, argv[i]);
            _print_test_domain(pslt, domain);
        }

        strcpy(domain, "google.co.uk");
        _print_test_domain(pslt, domain);

        strcpy(domain, "gmail.aksoak.aosk.google.co.uk");
        _print_test_domain(pslt, domain);
    }
    else
    if (!strcmp(command, "csvtest")) {
        pslt_csv_test(pslt, "../../asset/psltrie/test.csv");
    }


    pslt_trie_free(pslt);

    (void)(unused);

    return EXIT_SUCCESS;
}

