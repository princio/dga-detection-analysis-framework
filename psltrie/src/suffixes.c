#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psltrie.h"

#define ROW_NFIELDS 7

// suffix               54
// code                 10
// punycode             24
// type                 18
// origin               5
// section              14
// isprivate            7

int _suffixes_get_field_end(char * row, int cursor) {
    size_t cursor_end;
    
    char * location_end = strchr(row + cursor, ',');

    if (location_end) {
        cursor_end = location_end - row;
    } else {
        cursor_end = strlen(row);
    }

    if (cursor_end > strlen(row)) {
        printf("Something is wrong: %ld > %zu [str len]", cursor_end, strlen(row));
        cursor_end = strlen(row);
    }

    return cursor_end;
}

/* PUBLIC METHODS */

PSLTError pslt_suffixes_parse(char filepath[PATH_MAX], PSLTSuffixes* suffixes) {
    const size_t MAXFIELDSIZE = 500;
    const size_t MAXROWSIZE = ROW_NFIELDS * MAXFIELDSIZE;

    FILE* fp;
    size_t lines;
    char field[MAXFIELDSIZE];
    char buffer[MAXROWSIZE];
    size_t line_number;
    char* unused;

    memset(suffixes, 0, sizeof(PSLTSuffixes));
  
    fp = fopen(filepath, "r");
    if (!fp){
        printf("Can't open file\n");
        return PSLT_ERROR_SUFFIXLIST_FOPEN_ERROR;
    }

    lines = 0;
    while(fgets(buffer, ROW_NFIELDS * sizeof(field), fp)) {
        ++lines;
    }
    --lines;

    suffixes->_ = calloc(lines, sizeof(PSLTSuffix));

    suffixes->_[0].index = 5; // TODO: CHECK
    suffixes->_[10].index = 15; // TODO: CHECK

    rewind(fp);

    line_number = 0;
    unused = fgets(buffer, MAXROWSIZE, fp); // skip the header (one line)
    while(fgets(buffer, MAXROWSIZE, fp)) {
        #define PARSE_FIELD(NAME) { \
            cursor_end = _suffixes_get_field_end(buffer, cursor); \
            memset(field, 0, MAXFIELDSIZE); \
            memcpy(field, &buffer[cursor], cursor_end - cursor); \
            cursor = cursor_end + 1; }

        PSLTSuffix* suffix = &suffixes->_[line_number];
        int cursor = 0;
        int cursor_end = 0;

        {
            PARSE_FIELD(index);
            suffix->index = (size_t) atoi(field);
        }

        { // skip single labels
            suffix->nlabels = 0;
            for (int i = 0; i < 5; ++i) {
                cursor_end = _suffixes_get_field_end(buffer, cursor);
                suffix->nlabels += ((cursor_end - cursor) > 1);
                cursor = cursor_end + 1;
            }
        }

        { // suffix
            PARSE_FIELD(suffix);
            memcpy(suffix->suffix, field, strlen(field));
            suffix->suffix[strlen(field)] = '\0';
        }

        { // code
            PARSE_FIELD(code);
            memcpy(&suffix->code.section, field, strlen(field));
        }

        { // punycode
            PARSE_FIELD(punycode);
            suffix->is_punycode = strlen(field) > 0;
        }

        { // type
            PARSE_FIELD(type);
            if(strcmp(field, "country-code") == 0) {
                suffix->type = PSLT_SUFFIXTYPE_COUNTRYCODE;
            } else
            if(strcmp(field, "generic") == 0) {
                suffix->type = PSLT_SUFFIXTYPE_GENERIC;
            } else
            if(strcmp(field, "generic-restricted") == 0) {
                suffix->type = PSLT_SUFFIXTYPE_GENERICRESTRICTED;
            } else
            if(strcmp(field, "infrastructure") == 0) {
                suffix->type = PSLT_SUFFIXTYPE_INFRASTRUCTURE;
            } else
            if(strcmp(field, "sponsored") == 0) {
                suffix->type = PSLT_SUFFIXTYPE_SPONSORED;
            } else
            if(strcmp(field, "test") == 0) {
                suffix->type = PSLT_SUFFIXTYPE_TEST;
            }
        }

        { // origin
            PARSE_FIELD(origin);
            if(strcmp(field, "both") == 0) {
                suffix->origin = PSLT_SUFFIXORIGIN_BOTH;
            }
            else
            if(strcmp(field, "icann") == 0) {
                suffix->origin = PSLT_SUFFIXORIGIN_ICANN;
            }
            else
            if(strcmp(field, "PSL") == 0) {
                suffix->origin = PSLT_SUFFIXORIGIN_PSL;
            }
        }

        { // section
            PARSE_FIELD(section);
            if(strcmp(field, "icann") == 0) {
                suffix->section = PSLT_SUFFIXSECTION_ICANN;
            }
            else
            if(strcmp(field, "icann-new") == 0) {
                suffix->section = PSLT_SUFFIXSECTION_ICANNNEW;
            }
            else
            if(strcmp(field, "private-domain") == 0) {
                suffix->section = PSLT_SUFFIXSECTION_PRIVATEDOMAIN;
            }
        }

        { // isprivate
            PARSE_FIELD(isprivate);
            suffix->is_private = strcmp(field, "icann") != 0;
        }

        ++line_number;

        #undef PARSE_FIELD
    }

    suffixes->n = line_number;

    fclose(fp);

    (void)(unused);

    return PSLT_ERROR_NO;
}