#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "trie.h"

#define BUFFER_SIZE 1024
#define MAX_DOMAIN_SIZE 500


struct Domain {
    char domain[MAX_DOMAIN_SIZE];
    int is_malware;
    int is_dga;
    struct SuffixSearchResult suffixes;
};


int main() {
    FILE* fp;
    int lines;
    int line;
    struct Domain* domains;

    char buffer[BUFFER_SIZE];

    struct TrieNode *root = trie_load("iframe.csv");
  
    fp = fopen("dataset.txt", "r");
 
    if (!fp){
        printf("Can't open file\n");
        exit(1);
    }

    lines = 0; 
    while(fgets(buffer, BUFFER_SIZE, fp)) {
        ++lines;
    }

    lines -= 1; // ignoring header
    rewind(fp);
    fgets(buffer, BUFFER_SIZE, fp); // ignoring header

    line = 0;
    domains = (struct Domain*) calloc(lines, sizeof(struct Domain));
    while(fgets(buffer, BUFFER_SIZE, fp)) {
        char *field_begin;
        char *field_end;
        int len;
        char is[2];
        
        field_begin = buffer;
        field_end = strchr(field_begin, ',');
        field_end = field_end == NULL ? (buffer + strlen(buffer)) : field_end;

        len = field_end - field_begin;

        strncpy(domains[line].domain, buffer, len > MAX_DOMAIN_SIZE ? MAX_DOMAIN_SIZE : len);
        
        field_begin += len + 1;
        // field_end = strchr(field_begin, ',');
        // field_end = field_end == NULL ? (buffer + strlen(buffer)) : field_end;
        domains[line].is_malware = field_begin[0] - '0';
        
        // field_begin += len;
        // field_end = strchr(field_begin, ',');
        // field_end = field_end == NULL ? (buffer + strlen(buffer)) : field_end;
        domains[line].is_dga = field_begin[2] - '0';

        // printf("%-50s\t%d\t%d\n", domains[line].domain, domains[line].is_malware, domains[line].is_dga);

        domains[line].suffixes = trie_search(root, domains[line].domain, 0);

        // if (line > 10) break;

        ++line;
    }

    // for (int i = 0; i < lines; ++i) {
    //     struct Domain* domain = &domains[i];
    //     printf("%-50s\t%10s\t%10s\t%10s\t%5d\t%5d\n",
    //     domain->domain,
    //     domain->suffixes.tld ? domain->suffixes.tld->suffix->suffix : "not found",
    //     domain->suffixes.icann ? domain->suffixes.icann->suffix->suffix : "not found",
    //     domain->suffixes.private ? domain->suffixes.private->suffix->suffix : "not found",
    //     domain->is_malware,
    //     domain->is_dga);
    // }

    return 0;
}