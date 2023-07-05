#include <stdlib.h>
#include <stddef.h>

#define MAX_SUFFIX_SIZE 100
#define N_MAX_SUFFIXES 100000
#define MAX_LABEL_SIZE 63

// suffix               54
// code                 10
// punycode             24
// type                 18
// origin               5
// section              14
// isprivate            7
typedef struct StructRow {
    char index[20];
    char suffix[MAX_SUFFIX_SIZE]; // 63 bytes is the maximum
    char code[15];
    char punycode[24];
    char type[18];
    char origin[6];
    char section[16];
    char isprivate[8];
} Row;


typedef enum SuffixType {
    CountryCode,
    Generic,
    GenericRestricted,
    Infrastructure,
    Sponsored,
    Test
} SuffixType;

typedef enum _SuffixOrigin {
    Origin_Both,
    Origin_Icann,
    Origin_PSL
} SuffixOrigin;

typedef enum _SuffixSection {
    Section_Icann,
    Section_IcannNew,
    Section_PrivateDomain
} SuffixSection;


// ### Coding:
// ###
// ### A code is composed by: S00000TTE1
// ### Where:
// ### - S: the section (icann, icann-new, private-domain)
// ### - 00000: five digits which indicates the index in the etld frame
// ### - TT: two letters which indicates the type
// ### - E: if is an exception suffix
// ### - 1: the number of labels
typedef struct StructSuffixCode {
    char section;
    char index[5];
    char type[2];
    char is_exception;
    char nlabels;
} SuffixCode;

typedef struct _Suffix
{
    int index;

    char label0[MAX_LABEL_SIZE];
    char label1[MAX_LABEL_SIZE];
    char label2[MAX_LABEL_SIZE];
    char label3[MAX_LABEL_SIZE];
    char label4[MAX_LABEL_SIZE];

    char suffix[MAX_SUFFIX_SIZE];

    SuffixCode code;
    SuffixType type;
    SuffixOrigin origin;
    SuffixSection section;

    int is_punycode;
    int is_private;
    int is_exception;
    int nlabels;
} Suffix;


void load_suffixes(char*, Suffix **, int*);