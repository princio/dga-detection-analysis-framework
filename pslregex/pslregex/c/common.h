#ifndef __CSVPARSERDOMAIN__
#define __CSVPARSERDOMAIN__

#define MAX_LABEL_SIZE 63

#define MAX_DOMAIN_LABEL_NUM 20
#define MAX_SUFFIX_LABEL_NUM 6

#define MAX_DOMAIN_SIZE MAX_DOMAIN_LABEL_NUM * MAX_LABEL_SIZE + MAX_DOMAIN_LABEL_NUM
#define MAX_SUFFIX_SIZE MAX_SUFFIX_LABEL_NUM * MAX_LABEL_SIZE + MAX_SUFFIX_LABEL_NUM

#define N_MAX_SUFFIXES 100000


typedef char DomainFull[MAX_DOMAIN_SIZE];
typedef char DomainLabel[MAX_LABEL_SIZE];
typedef char DomainSuffix[MAX_SUFFIX_SIZE];

struct Domain {
    DomainFull domain;
    int is_malware;
    int is_dga;
    struct SuffixSearchResult suffixes;
};

#endif