#ifndef __PSLREGEX2_COMMON__
#define __PSLREGEX2_COMMON__

#define DOMAIN_ALPHABET_SIZE 484

#define MAX_LABEL_SIZE 63

#define MAX_DOMAIN_LABEL_NUM 20
#define MAX_SUFFIX_LABEL_NUM 6

#define MAX_DOMAIN_SIZE MAX_DOMAIN_LABEL_NUM * MAX_LABEL_SIZE + MAX_DOMAIN_LABEL_NUM
#define MAX_SUFFIX_SIZE MAX_SUFFIX_LABEL_NUM * MAX_LABEL_SIZE + MAX_SUFFIX_LABEL_NUM

#define N_MAX_SUFFIXES 100000


typedef char DomainFull[MAX_DOMAIN_SIZE];
typedef char DomainLabel[MAX_LABEL_SIZE];
typedef char DomainSuffix[MAX_SUFFIX_SIZE];

typedef enum SuffixType {
    SUFFIXTYPE_COUNTRYCODE,
    SUFFIXTYPE_GENERIC,
    SUFFIXTYPE_GENERICRESTRICTED,
    SUFFIXTYPE_INFRASTRUCTURE,
    SUFFIXTYPE_SPONSORED,
    SUFFIXTYPE_TEST
} SuffixType;

typedef enum SuffixOrigin {
    SUFFIXORIGIN_BOTH,
    SUFFIXORIGIN_ICANN,
    SUFFIXORIGIN_PSL
} SuffixOrigin;

typedef enum SuffixSection {
    SUFFIXSECTION_ICANN,
    SUFFIXSECTION_ICANNNEW,
    SUFFIXSECTION_PRIVATEDOMAIN
} SuffixSection;


typedef struct SuffixCode { // A code like: "S00000TTE1"
    char section; // S: the section (icann, icann-new, private-domain)
    char index[5]; // 00000: five digits which indicates the index in the etld frame
    char type[2]; // TT: two letters which indicates the type
    char is_exception; // E: if is an exception suffix
    char nlabels; // 1: the number of labels
} SuffixCode;

typedef struct Suffix {
    size_t index;

    DomainSuffix suffix;

    SuffixCode code;
    SuffixType type;
    SuffixOrigin origin;
    SuffixSection section;

    int is_punycode;
    int is_private;
    int is_exception;
    int nlabels;
} Suffix;

typedef struct DomainSuffixes {
    Suffix tld;
    Suffix icann;
    Suffix private;
} DomainSuffixes;

struct Domain {
    DomainFull domain;
    int is_malware;
    int is_dga;

    DomainSuffixes suffixes;
};


int domain_nlabels(const char domain[]);

int domain_labels(const char domain[], char labels[][64]);

int domain_invert(const char domain[], char inverted[]);

#endif