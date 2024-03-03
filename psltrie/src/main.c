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

    PSLTSuffixes suffixes;
    PSLTNode* trie;
    
    PSLTError error = pslt_suffixes_parse(argv[1], &suffixes);

    if (error) {
        fprintf(stderr, "Error during opening file: %s", strerror(errno));
        exit(1);
    }

    plst_generate(suffixes, &trie);

    if (trie == NULL) {
        fprintf(stderr, "Impossible to load the prefix tree.");
        exit(1);
    }

    PSLTSuffixSearchResult result = pslt_search(trie, argv[2]);

    printf("    tld: %s\n", result.tld ? result.tld->suffix->suffix : "-");
    printf("  icann: %s\n", result.icann ? result.icann->suffix->suffix : "-");
    printf("private: %s\n", result.private ? result.private->suffix->suffix : "-");

    pslt_free(trie);
    free(suffixes._);

    return EXIT_SUCCESS;
}

