#include "psltrie.h"

#include <string.h>

int pslt_count_domain_labels(PSLTDomainFull domain) {
    const char* begin;
    const char* end;

    int nlabels = 0;

    begin = domain;
    end = domain;
    while ((end = strchr(begin, '.')) ) {
        ++nlabels;
        begin = end + 1;
    }
    return nlabels + 1;
}

int pslt_domain_labels(PSLTDomainFull domain, PSLTDomainLabels labels) {
    const char* begin;
    const char* end;

    memset(labels, 0, 10 * 64);

    int nlabels;

    begin = domain;
    end = domain;
    nlabels = 0;
    while ( (end = strchr(begin, '.')) == NULL ) {
        strncpy(labels[nlabels], begin, end - begin);
        ++nlabels;
        begin = end + 1;
    }

    return nlabels;
}

int pslt_domain_invert(PSLTDomainFull domain, PSLTDomainFull inverted) {
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
