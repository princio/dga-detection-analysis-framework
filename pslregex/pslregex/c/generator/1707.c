#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../suffixes/trie.h"

#define BUFFER_SIZE 1024
#define MAX_DOMAIN_SIZE 500

#define PATH "/home/princio/Repo/princio/malware_detection_preditc_file/pslregex/pslregex/c/dataset.txt"

struct Domain {
    char domain[MAX_DOMAIN_SIZE];
    char class[50];
    char labels[10][64];
    int nlabels;
    struct SuffixSearchResult suffixes;
    int is_malware;
    int is_dga;
};


struct XOutput {
    char domain[MAX_DOMAIN_SIZE];
    char tld[MAX_DOMAIN_SIZE];
    char icann[MAX_DOMAIN_SIZE];
    char private[MAX_DOMAIN_SIZE];
    char first_17char[MAX_DOMAIN_SIZE];
    char first_17char_tld[MAX_DOMAIN_SIZE];
    char first_17char_icann[MAX_DOMAIN_SIZE];
    char first_17char_private[MAX_DOMAIN_SIZE];
    int is_dga;
    int is_malware;
};


static int get_file_lines(FILE *fp, int skip_header, int *max_buffer_length) {
    rewind(fp);
    char ch;
    int lines = 0;
    int max = 0;
    int ch_per_line = 0;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n') {
            ++lines;
            if (max < ch_per_line) max = ch_per_line;
            ch_per_line = 0;
        }
        ++ch_per_line;
    }
    *max_buffer_length = max;
    return lines - skip_header;
}

static void parse_file(struct Domain domains[], FILE* fp, struct TrieNode *root, int max_buffer_length) {

    max_buffer_length += 5;

    char buffer[max_buffer_length];

    int line = 0;
    while(fgets(buffer, max_buffer_length, fp)) {
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
}

static void generate_X(const struct Domain* domains, const int ndomains) {
    struct XOutput* xoutputs = calloc(ndomains, sizeof(struct XOutput));

    for (int i = 0; i < ndomains; ++i) {
        const struct Domain* domain = &domains[i];
        struct XOutput* xout = &xoutputs[i];
        int nch;
        int l = strlen(domain->domain);

        strcpy(xout->domain, domain->domain);

        {
            if (domain->suffixes.tld) {
                nch = strlen(domain->domain) - strlen(domain->suffixes.tld->suffix->suffix) - 1;
                if (nch > 0)
                    strncpy(xout->tld, domain->domain, nch);
                else
                    strncpy(xout->tld, "NaN\0", 4);
            } else {
                strcpy(xout->tld, domain->domain);
            }
        }

        {
            if (domain->suffixes.icann) {
                nch = strlen(domain->domain) - strlen(domain->suffixes.icann->suffix->suffix) - 1;
                if (nch > 0)
                    strncpy(xout->icann, domain->domain, nch);
                else
                    strncpy(xout->icann, "NaN\0", 4);
            } else {
                strcpy(xout->icann, xout->tld);
            }
        }

        {
            if (domain->suffixes.private) {
                nch = strlen(domain->domain) - strlen(domain->suffixes.private->suffix->suffix) - 1;
                if (nch > 0)
                    strncpy(xout->private, domain->domain, nch);
                else
                    strncpy(xout->private, "NaN\0", 4);
            } else {
                strcpy(xout->private, xout->icann);
            }
        }

        {   
            l = strlen(domain->domain);
            nch = 17;
            int start = l - 17;
            if (start < 0) {
                nch = 17 + start;
                start = 0;
            }
            strncpy(xout->first_17char, xout->domain + start, nch);
            xout->first_17char[xout->first_17char[nch - 1] == '.' ? nch - 1 : nch] = '\0';
        }

        {   
            l = strlen(xout->tld);
            nch = 17;
            int start = l - 17;
            if (start < 0) {
                nch = 17 + start;
                start = 0;
            }
            strncpy(xout->first_17char_tld, xout->tld + start, nch);
            xout->first_17char[xout->first_17char[nch - 1] == '.' ? nch - 1 : nch] = '\0';
        }

        {   
            l = strlen(xout->icann);
            nch = 17;
            int start = l - 17;
            if (start < 0) {
                nch = 17 + start;
                start = 0;
            }
            strncpy(xout->first_17char_icann, xout->icann + start, nch);
            xout->first_17char[xout->first_17char[nch - 1] == '.' ? nch - 1 : nch] = '\0';
        }

        {   
            l = strlen(xout->private);
            nch = 17;
            int start = l - 17;
            if (start < 0) {
                nch = 17 + start;
                start = 0;
            }
            strncpy(xout->first_17char_private, xout->private + start, nch);
            xout->first_17char[xout->first_17char[nch - 1] == '.' ? nch - 1 : nch] = '\0';
        }

        xout->is_malware = domain->is_malware;
        xout->is_dga = domain->is_dga;
    }


    FILE* fp = fopen("X.csv", "w");

    fprintf(
        fp,
        ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        "none_X",
        "tld_X",
        "icann_X",
        "private_X",
        "none_first_17char_X",
        "tld_first_17char_X",
        "icann_first_17char_X",
        "private_first_17char_X",
        "is_malware",
        "is_dga"
    );

    for (unsigned  i = 0; i < ndomains; ++i) {
        struct XOutput xout = xoutputs[i];
        fprintf(
            fp,
            "%u,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d\n",
            i,
            xout.domain,
            xout.tld,
            xout.icann,
            xout.private,
            xout.first_17char,
            xout.first_17char_tld,
            xout.first_17char_icann,
            xout.first_17char_private,
            xout.is_malware,
            xout.is_dga
        );
    }

    fflush(fp);

    fclose(fp);
}

int fs1707_run() {
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

    int max_buffer_length;
    const int LINES = get_file_lines(fp, 1, &max_buffer_length);

    rewind(fp);
    fgets(buffer, BUFFER_SIZE, fp); // skipping header

    line = 0;
    domains = (struct Domain*) calloc(LINES, sizeof(struct Domain));

    parse_file(domains, fp, root, max_buffer_length);

    generate_X(domains, LINES);

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