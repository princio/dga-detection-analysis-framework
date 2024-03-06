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

#include <getopt.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main (int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments [suffixlist.csv] [domain]");
        exit(1);
    }

    PSLT* pslt;

    pslt = pslt_load(argv[1]);

    pslt_print(pslt);

    if (pslt == NULL) {
        fprintf(stderr, "Something went wrong.");
        exit(1);
    }

    PSLTSuffixSearchResult result = pslt_search(pslt, argv[2]);

    printf("    tld: %s\n", result.tld ? result.tld->suffix->suffix : "-");
    printf("  icann: %s\n", result.icann ? result.icann->suffix->suffix : "-");
    printf("private: %s\n", result.private ? result.private->suffix->suffix : "-");

    PSLTDomainProcessed processed = pslt_domain_remove_suffixes(argv[2], result);

    printf("    tld: %s\n", processed.tld);
    printf("  icann: %s\n", processed.icann);
    printf("private: %s\n", processed.private);

    {
        PSLTDomain tmp = "google.co.uk";
        PSLTDomain bdn;
        pslt_basedomain(tmp, result, bdn);
        printf("    bdn: %s -> %s\n", tmp, bdn);
    }
    {
        PSLTDomain tmp = "gmail.aksoak.aosk.google.co.uk";
        PSLTDomain bdn;
        pslt_basedomain(tmp, result, bdn);
        printf("    bdn: %s -> %s\n", tmp, bdn);
    }

    pslt_free(pslt);

    return EXIT_SUCCESS;
}

