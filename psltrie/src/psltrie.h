#ifndef __PSLREGEX2_COMMON__
#define __PSLREGEX2_COMMON__

#include <stdio.h>
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


#define N_PSLTSUFFIXGROUP 3
#define PSLT_FOR_GROUP(V) for (size_t (V) = 0; (V) < N_PSLTSUFFIXGROUP; (V)++)

typedef enum PSLTSuffixGroup {
    PSLT_GROUP_TLD,
    PSLT_GROUP_ICANN,
    PSLT_GROUP_PRIVATE
} PSLTSuffixGroup;

typedef struct PSLTSuffix {
    size_t index;
    int is_ascii;
    PSLTDomainSuffix suffix;
    int nlabels;
    PSLTSuffixGroup group;
} PSLTSuffix;

typedef struct PSLTSuffixes {
    size_t n;
    PSLTSuffix* _;
} PSLTSuffixes;

typedef struct PSLTNode { 
    struct PSLTNode* parent;
    
    size_t nbranches; // just for the next level
    size_t nchildren; // for all sublevels
    struct PSLTNode* children[PSLT_DOMAIN_ALPHABET_SIZE];

    int abc_index;
    int depth;

    PSLTDomainSuffix path;
    PSLTDomainSuffix key;

    PSLTSuffix* suffix;
    PSLTSuffix inverted;

    void* compacted;
} PSLTNode;

typedef struct PSLT {
    PSLTSuffixes suffixes;
    PSLTNode* trie;
} PSLT;

typedef struct PSLTObject {
    PSLT* pslt;
    PSLTDomain domain;

    int searched;

    PSLTDomain basedomain;

    PSLTNode* suffixes[N_PSLTSUFFIXGROUP];

    PSLTDomain suffixless[N_PSLTSUFFIXGROUP];

} PSLTObject;


int pslt_domain_count_labels(PSLTDomain domain);
int pslt_domain_labels(PSLTDomain domain, PSLTDomainLabels labels);
int pslt_domain_invert(PSLTDomain domain, PSLTDomain inverted);

PSLT* pslt_trie_load(char suffixlistpath[PATH_MAX]);
void pslt_trie_print(PSLT* pslt);
void pslt_trie_free(PSLT* pslt);
int pslt_domain_run(PSLT* pslt, PSLTObject* obj);

int pslt_csv(PSLT* pslt, int dn_col, char csv_in_path[PATH_MAX], char csv_out_path[PATH_MAX]);
int pslt_csv_test(PSLT* pslt, char csvtest_path[PATH_MAX]);


extern FILE* pslt_logger_file;

#define LOG_ERROR(F, ...) fprintf(pslt_logger_file, "[erro]: "); fprintf(pslt_logger_file, (F), ##__VA_ARGS__); fprintf(pslt_logger_file, "\n");
#define LOG_WARN(F, ...) fprintf(pslt_logger_file, "[warn]: "); fprintf(pslt_logger_file, (F), ##__VA_ARGS__); fprintf(pslt_logger_file, "\n");
#define LOG_INFO(F, ...) fprintf(pslt_logger_file, "[info]: "); fprintf(pslt_logger_file, (F), ##__VA_ARGS__); fprintf(pslt_logger_file, "\n");
#define LOG_DEBUG(F, ...) fprintf(pslt_logger_file, "[debug]: "); fprintf(pslt_logger_file, (F), ##__VA_ARGS__); fprintf(pslt_logger_file, "\n");
#define LOG_TRACE(F, ...) fprintf(pslt_logger_file, "[trace]: "); fprintf(pslt_logger_file, (F), ##__VA_ARGS__); fprintf(pslt_logger_file, "\n");

#endif