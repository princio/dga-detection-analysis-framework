#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stddef.h>

#include "utils.h"

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


// ### Coding:
// ###
// ### A code is composed by: S00000TTE1
// ### Where:
// ### - S: the section (icann, icann-new, private-domain)
// ### - 00000: five digits which indicates the index in the etld frame
// ### - TT: two letters which indicates the type
// ### - E: if is an exception suffix
// ### - 1: the number of labels
typedef struct SuffixCode {
    char section;
    char index[5];
    char type[2];
    char is_exception;
    char nlabels;
} SuffixCode;

typedef struct Suffix
{
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


void parser(char*, Suffix **, int*);

#endif