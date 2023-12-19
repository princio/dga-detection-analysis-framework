#include "common.h"

#include <string.h>

int domain_nlabels(const char domain[]) {
    const char* begin;
    const char* end;

    int nlabels = 0;
    int nlengths[10];

    begin = domain;
    end = domain;
    while (end = strchr(begin, '.')) {
        nlengths[nlabels] = end - begin;
        ++nlabels;
        begin = end + 1;
    }
    return nlabels + 1;
}

int domain_labels(const char domain[], char labels[][64]) {
    const char* begin;
    const char* end;

    int nlabels = 0;
    int nlengths[10];

    begin = domain;
    end = domain;
    while (end = strchr(begin, '.')) {
        nlengths[nlabels] = end - begin;
        ++nlabels;
        begin = end + 1;
    }
    nlengths[nlabels] = strlen(begin);
    const int NLABELS = nlabels + 1;

    begin = domain;
    for (int i = 0; i < NLABELS; ++i) {
        strncpy(labels[i], begin, nlengths[i]);
        labels[i][nlengths[i]] = '\0';
        begin += nlengths[i] + 1;
    }
}

int domain_invert(const char domain[], char inverted[]) {
    memset(inverted, 0, strlen(domain) + 1);

    int len = strlen(domain);

    for(int i = len - 1; i >=0; ) {
        int dot_position;
        int domain_cursor;
        int inv_cursor;
        int label_len;
        
        dot_position = i - 1;
        while(domain[dot_position] != '.' && dot_position > 0) { --dot_position; }

        domain_cursor = dot_position + (dot_position > 0);
        inv_cursor = (len - i - 1);
        label_len = i - dot_position + (dot_position== 0);

        strncpy(inverted + inv_cursor, domain + domain_cursor, label_len);

        if (dot_position > 0)
            inverted[inv_cursor + label_len] = '.';

        i = dot_position - 1;
    }

    return 0;
}
