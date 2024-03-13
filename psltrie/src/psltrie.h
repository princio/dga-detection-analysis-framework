#ifndef __PSLREGEX2_COMMON__
#define __PSLREGEX2_COMMON__

#include <stdlib.h>
#include <linux/limits.h>


#define PSLT_LIST_ROW_FIELDS_NUMBER 7

// suffix               54
// code                 10
// punycode             24
// type                 18
// origin               5
// section              14
// isprivate            7

#define PSLT_DOMAIN_ALPHABET_SIZE 484

#define PSLT_MAX_LABEL_SIZE 63

#define PSLT_MAX_DOMAIN_LABEL_NUM 20
#define PSLT_MAX_SUFFIX_LABEL_NUM 10 // max is 6

#define PSLT_MAX_DOMAIN_SIZE PSLT_MAX_DOMAIN_LABEL_NUM * PSLT_MAX_LABEL_SIZE + PSLT_MAX_DOMAIN_LABEL_NUM
#define PSLT_MAX_SUFFIX_SIZE PSLT_MAX_SUFFIX_LABEL_NUM * PSLT_MAX_LABEL_SIZE + PSLT_MAX_SUFFIX_LABEL_NUM

#define PSLT_N_MAX_SUFFIXES 100000

typedef enum PSLTError {
    PSLT_ERROR_NO = 0,
    PSLT_ERROR_SUFFIXLIST_FOPEN_ERROR,
} PSLTError;

typedef char PSLTDomain[PSLT_MAX_DOMAIN_SIZE];
typedef char PSLTDomainLabel[PSLT_MAX_LABEL_SIZE];
typedef char PSLTDomainSuffix[PSLT_MAX_SUFFIX_SIZE];
typedef char PSLTDomainSuffixLabeled[PSLT_MAX_SUFFIX_LABEL_NUM][PSLT_MAX_LABEL_SIZE];
typedef char PSLTDomainLabels[PSLT_MAX_DOMAIN_LABEL_NUM][PSLT_MAX_LABEL_SIZE];

typedef enum PSLTSuffixType {
    PSLT_SUFFIXTYPE_COUNTRYCODE,
    PSLT_SUFFIXTYPE_GENERIC,
    PSLT_SUFFIXTYPE_GENERICRESTRICTED,
    PSLT_SUFFIXTYPE_INFRASTRUCTURE,
    PSLT_SUFFIXTYPE_SPONSORED,
    PSLT_SUFFIXTYPE_TEST
} PSLTSuffixType;

typedef enum PSLTSuffixOrigin {
    PSLT_SUFFIXORIGIN_BOTH,
    PSLT_SUFFIXORIGIN_ICANN,
    PSLT_SUFFIXORIGIN_PSL
} PSLTSuffixOrigin;

typedef enum SuffixSection {
    PSLT_SUFFIXSECTION_ICANN,
    PSLT_SUFFIXSECTION_ICANNNEW,
    PSLT_SUFFIXSECTION_PRIVATEDOMAIN
} SuffixSection;


typedef struct PSLTSuffixCode { // A code like: "S00000TTE1"
    char section; // S: the section (icann, icann-new, private-domain)
    char index[5]; // 00000: five digits which indicates the index in the etld frame
    char type[2]; // TT: two letters which indicates the type
    char is_exception; // E: if is an exception suffix
    char nlabels; // 1: the number of labels
} PSLTSuffixCode;

typedef struct PSLTSuffix {
    size_t index;

    PSLTDomainSuffix suffix;

    PSLTSuffixCode code;
    PSLTSuffixType type;
    PSLTSuffixOrigin origin;
    SuffixSection section;

    int is_punycode;
    int is_newGLTD;
    int is_private;
    int is_exception;
    int nlabels;
} PSLTSuffix;

typedef struct PSLTSuffixes {
    size_t n;
    PSLTSuffix* _;
} PSLTSuffixes;

typedef struct PSLTDomainSuffixes {
    PSLTSuffix tld;
    PSLTSuffix icann;
    PSLTSuffix private;
} PSLTDomainSuffixes;

struct PSLTDomain {
    PSLTDomain domain;
    int is_malware;
    int is_dga;

    PSLTDomainSuffixes suffixes;
};

typedef enum PSLTSuffixSearchHow {
    SuffixSearchHow_TLD,
    SuffixSearchHow_ICANN,
    SuffixSearchHow_PRIVATE
} PSLTSuffixSearchHow;

typedef struct PSLTNode { 
    struct PSLTNode *children[PSLT_DOMAIN_ALPHABET_SIZE];
    int isEndOfSuffix;
    PSLTSuffix* suffix;
    PSLTSuffix inverted;
} PSLTNode;

typedef struct PSLTTriesSuffixes {
    struct PSLTNode *tld;
    struct PSLTNode *icann;
    struct PSLTNode *private;
} PSLTTriesSuffixes;

typedef struct PSLTDomainProcessed {
    PSLTDomain tld;
    PSLTDomain icann;
    PSLTDomain private;
} PSLTDomainProcessed;

typedef struct PSLTResult {
    PSLTDomain domain;
    PSLTDomain basedomain;
    PSLTTriesSuffixes suffixes;
    PSLTDomainProcessed processed;
} PSLTResult;

typedef struct PSLT {
    PSLTSuffixes suffixes;
    PSLTNode* trie;
} PSLT;


typedef struct PSLTObject {
    PSLT* pslt;
    PSLTDomain domain;

    int searched;

    PSLTDomain basedomain;
    PSLTTriesSuffixes suffixes;
    struct {
        PSLTDomain tld;
        PSLTDomain icann;
        PSLTDomain private;
    } wo_suffixes;
} PSLTObject;


int pslt_domain_count_labels(PSLTDomain domain);
int pslt_domain_labels(PSLTDomain domain, PSLTDomainLabels labels);
int pslt_domain_invert(PSLTDomain domain, PSLTDomain inverted);

PSLT* pslt_trie_load(char suffixlistpath[PATH_MAX]);
void pslt_trie_print(PSLT* pslt);
void pslt_trie_free(PSLT* pslt);
int pslt_domain_run(PSLT* pslt, PSLTObject* obj);

int pslt_csv(PSLT* pslt, int dn_col, char csv_in_path[PATH_MAX], char csv_out_path[PATH_MAX]);


#endif