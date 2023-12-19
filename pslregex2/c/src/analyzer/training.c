#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../suffixes/trie.h"

#include "training.h"

#define BUFFER_SIZE 1024
#define MAX_DOMAIN_SIZE 500

#define PATH "/home/princio/Repo/princio/malware_detection_preditc_file/pslregex/pslregex/c/dataset_training.csv"

struct Domain {
    char domain[MAX_DOMAIN_SIZE];
    int is_malware;
    char class[50];
    char labels[10][64];
    int nlabels;
    struct SuffixSearchResult suffixes;
};


struct XOutput {
    char domain[MAX_DOMAIN_SIZE];
    int is_malware;
    char class[50];
    char labels[10][64];
    int nlabels;
    struct SuffixSearchResult suffixes;
};


int training_analyze() {
    FILE* fp;
    int line;
    int lines = 0; 
    struct Domain* domains;

    char buffer[BUFFER_SIZE];

    struct TrieNode *root = trie_load("iframe.csv");
  
    fp = fopen(PATH, "r");
 
    if (!fp){
        printf("Can't open file\n");
        exit(1);
    }

    while(fgets(buffer, BUFFER_SIZE, fp)) {
        ++lines;
    }

    const int LINES = lines - 1; // skipping header

    rewind(fp);
    fgets(buffer, BUFFER_SIZE, fp); // skipping header

    line = 0;
    domains = (struct Domain*) calloc(LINES, sizeof(struct Domain));
    while(fgets(buffer, BUFFER_SIZE, fp)) {
        char *field_begin;
        char *field_end;
        int len;
        char is[2];
        
        field_begin = buffer;

        /* first field - LEGIT */
        {
            char tmp[20];
            field_end = strchr(field_begin, ',');
            if (field_end == NULL) field_end = buffer + strlen(buffer);
            len = field_end - field_begin;
            strncpy(tmp, field_begin, len);
            tmp[len] = '\0';
            domains[line].is_malware = strcmp(tmp, "dga") == 0;
            field_begin += len + 1;
        }
        
        
        /* second field - CLASS */
        field_end = strchr(field_begin, ',');
        if (field_end == NULL) field_end = buffer + strlen(buffer);
        len = field_end - field_begin;
        strncpy(domains[line].class, field_begin, len);
        domains[line].class[len] = '\0';
        field_begin += len + 1;
        
        
        /* third field - DOMAIN */
        field_end = strchr(field_begin, ',');
        if (field_end == NULL) field_end = buffer + strlen(buffer);
        len = field_end - field_begin;
        strncpy(domains[line].domain, field_begin, len > MAX_DOMAIN_SIZE ? MAX_DOMAIN_SIZE : len);
        domains[line].domain[len - 1] = '\0'; // watchout the newline char

        domains[line].nlabels = domain_nlabels(domains[line].domain);
        domain_labels(domains[line].domain, domains[line].labels);

        if (line < 5) {
            printf("\n%-50s\n", domains[line].domain);
            for (int j = 0; j < domains[line].nlabels; ++j) {
                printf("%s\n", domains[line].labels[j]);
            }
            printf("\n%-50s\t%d\t%s\n", domains[line].domain, domains[line].is_malware, domains[line].class);
        }

        domains[line].suffixes = trie_search(root, domains[line].domain, 0);

        ++line;
    }

    // for (int i = 0; i < lines; ++i) {
    //     struct Domain* domain = &domains[i];
    //     printf("%-70s\t%10s\t%10s\t%10s\t%5d\n",
    //     domain->domain,
    //     domain->suffixes.tld ? domain->suffixes.tld->suffix->suffix : "not found",
    //     domain->suffixes.icann ? domain->suffixes.icann->suffix->suffix : "not found",
    //     domain->suffixes.private ? domain->suffixes.private->suffix->suffix : "not found",
    //     domain->is_malware);
    // }

    int len_total = 0;
    int len_total_dga = 0;
    int len_total_legit = 0;
    for (int i = 0; i < LINES; ++i) {
        struct Domain* domain = &domains[i];

        const int l = strlen(domain->domain);

        len_total += l;

        if (domain->is_malware) {
            len_total_dga += l;
        } else {
            len_total_legit += l;
        }
    }

    printf("%f\n", (float) len_total / lines);
    printf("%f\n", (float) len_total_dga / (lines / 2));
    printf("%f\n", (float) len_total_legit / (lines / 2));
    
    return 0;
}