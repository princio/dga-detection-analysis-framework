#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

// suffix               54
// code                 10
// punycode             24
// type                 18
// origin               5
// section              14
// isprivate            7

typedef struct StructRow {
    char index[20];
    char suffix; // 63 bytes is the maximum
    char code[15];
    char punycode[24];
    char type[18];
    char origin[6];
    char section[16];
    char isprivate[8];
} Row;

int read_header()
{
    // Substitute the full file path
    // for the string file_path
    FILE* fp = fopen("iframe.csv", "r");
 
    if (!fp)
        printf("Can't open file\n");
 
    else {
        // Here we have taken size of
        // array 1024 you can modify it
 
        int row = 0;
        int column = 0;

        char *line = NULL;
        int header_max_length = 0;
        int header_num = 0;
        char *headers_pointer[100];
        {
            size_t len = 0;
            size_t read;

            read = getline(&line, &len, fp);

            if (read == -1) {
                exit(EXIT_FAILURE);
            }

            printf("Retrieved line of length %zu :\n", read);
            printf("%s\n\n", line);

            char* value = strtok(line, ",");
            printf(": %s\n", value);
            while (value) {
                printf(": %s\n", value);
                if (header_max_length < strlen(value)) {
                    header_max_length = strlen(value) + 1;
                }
                headers_pointer[header_num] = value;
                ++header_num;
                value = strtok(NULL, ",");
            }
        }
        printf("header_max_length %d\n", header_max_length);
        printf("header_num %d\n", header_num);

        char header[header_num][header_max_length];
        memset(header, 0, header_num * header_max_length);
        {
            for (int i = 0; i < header_num; ++i) {
                int copy_nchars = strlen(headers_pointer[i]);
                if (headers_pointer[i][copy_nchars - 1] == '\n') {
                    --copy_nchars;
                }
                strncpy(header[i], headers_pointer[i], copy_nchars);
                printf("~%10d\t\t%s~\n", i, header[i]);
            }
        }

        int columns_max_length[header_num];
        {

        }

        exit(EXIT_SUCCESS);

        // Close the file
        fclose(fp);
    }
    return 0;
}

int get_field_end(char * row, int cursor) {
    int cursor_end;
    
    char * location_end = strchr(row + cursor, ',');

    if (location_end) {
        cursor_end = location_end - row;
    } else {
        cursor_end = strlen(row);
    }

    if (cursor_end > strlen(row)) {
        printf("Something is wrong: %d > %zu [str len]", cursor_end, strlen(row));
        cursor_end = strlen(row);
    }

    return cursor_end;
}



/* PUBLIC METHODS */

void parser(char *filepath, Suffix** suffixes_ptr, int *nsuffixes) {

    Row row;
    FILE* fp;
    int lines; // header count;

    char buffer[sizeof(row)];
  
    fp = fopen(filepath, "r");
 
    if (!fp){
        printf("Can't open file\n");
        exit(1);
    }

    lines = -1;
    while(fgets(buffer, sizeof(row), fp)) {
        ++lines;
    }

    Suffix* suffixes = calloc(lines, sizeof(Suffix));

    suffixes[0].index = 5;
    suffixes[10].index = 15;

    rewind(fp);

    int line_number = 0;
    fgets(buffer, sizeof(row), fp); // skip the header
    while(fgets(buffer, sizeof(row), fp)) {

        char tmp[100];

        Suffix* suffix = &suffixes[line_number];
        int cursor = 0;
        int cursor_end = 0;

        memset(&row, 0, sizeof(row));
        
        cursor_end = get_field_end(buffer, cursor);
        memcpy(&row.index, &buffer[cursor], cursor_end - cursor);
        cursor = cursor_end + 1;

        // skip single labels
        int nlabels = 0;
        for (int i = 0; i < 5; ++i) {
            cursor_end = get_field_end(buffer, cursor);
            nlabels += cursor_end - cursor > 1;
            cursor = cursor_end + 1;
        }

        // suffix
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.suffix, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.suffix);
        }

        // code
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.code, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.code);
        }

        // punycode
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.punycode, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.punycode);
        }

        // type
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.type, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.type);
        }

        // origin
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.origin, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.origin);
        }

        // section
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.section, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.section);
        }

        // isprivate
        {
            int l;
            cursor_end = get_field_end(buffer, cursor);
            l = cursor_end - cursor;
            memcpy(&row.isprivate, &buffer[cursor], l);
            tmp[l + 1] = '\0';
            cursor = cursor_end + 1;
            // printf("%s\n", row.isprivate);
        }

        // printf("\n");
        // printf("%s", buffer);
        // printf("%-10s\t%s\n", "index", row.index);
        // printf("%-10s\t%s\n", "suffix", row.suffix);
        // printf("%-10s\t%s\n", "code", row.code);
        // printf("%-10s\t%s\n", "punycode", row.punycode);
        // printf("%-10s\t%s\n", "type", row.type);
        // printf("%-10s\t%s\n", "origin", row.origin);
        // printf("%-10s\t%s\n", "section", row.section);
        // printf("%-10s\t%s\n", "isprivate", row.isprivate);


        sscanf(row.index, "%d", &(suffix->index));

        memcpy(suffix->suffix, row.suffix, strlen(row.suffix));
        suffix->suffix[strlen(row.suffix)] = '\0';

        memcpy(&suffix->code.section, row.code, strlen(row.code));

        suffix->is_punycode = strlen(row.punycode) > 0;

        if(strcmp(row.type, "country-code") == 0) {
            suffix->type = SUFFIXTYPE_COUNTRYCODE;
        } else
        if(strcmp(row.type, "generic") == 0) {
            suffix->type = SUFFIXTYPE_GENERIC;
        } else
        if(strcmp(row.type, "generic-restricted") == 0) {
            suffix->type = SUFFIXTYPE_GENERICRESTRICTED;
        } else
        if(strcmp(row.type, "infrastructure") == 0) {
            suffix->type = SUFFIXTYPE_INFRASTRUCTURE;
        } else
        if(strcmp(row.type, "sponsored") == 0) {
            suffix->type = SUFFIXTYPE_SPONSORED;
        } else
        if(strcmp(row.type, "test") == 0) {
            suffix->type = SUFFIXTYPE_TEST;
        }


        if(strcmp(row.origin, "both") == 0) {
            suffix->origin = SUFFIXORIGIN_BOTH;
        }
        else
        if(strcmp(row.origin, "icann") == 0) {
            suffix->origin = SUFFIXORIGIN_ICANN;
        }
        else
        if(strcmp(row.origin, "PSL") == 0) {
            suffix->origin = SUFFIXORIGIN_PSL;
        }


        
        if(strcmp(row.section, "icann") == 0) {
            suffix->section = SUFFIXSECTION_ICANN;
        }
        else
        if(strcmp(row.section, "icann-new") == 0) {
            suffix->section = SUFFIXSECTION_ICANNNEW;
        }
        else
        if(strcmp(row.section, "private-domain") == 0) {
            suffix->section = SUFFIXSECTION_PRIVATEDOMAIN;
        }


        suffix->is_private = strcmp(row.isprivate, "icann") != 0;

        suffix->nlabels = nlabels;


        // printf("\n");
        // printf("%-10s\t%d\n", "index", suffix.index);
        // printf("%-10s\t%zu\t%s\n", "suffix", strlen(suffix.suffix), suffix.suffix);
        // printf("%-10s\t%s\n", "code", (char*) &suffix.code);
        // printf("%-10s\t%d\n", "type", suffix.type);
        // printf("%-10s\t%d\n", "origin", suffix.origin);
        // printf("%-10s\t%d\n", "section", suffix.section);
        // printf("%-10s\t%d\n", "is_private", suffix.is_private);
        // printf("%-10s\t%d\n", "nlabels", suffix.nlabels);
        // printf("\n");
        // printf("\n");
        // printf("\n");
        // printf("\n");

        // value = strtok(NULL, ","); // 0
        // printf("0) %s\n", value);
        // value = strtok(NULL, ","); // 1
        // printf("1) %s\n", value);
        // value = strtok(NULL, ","); // 2
        // printf("2) %s\n", value);
        // value = strtok(NULL, ","); // 3
        // printf("3) %s\n", value);
        // value = strtok(NULL, ","); // 4
        // printf("4) %s\n", value);
        // value = strtok(NULL, ","); // suffix
        // strcpy(&suffix.suffix, value);
        // printf("suffix) %s\n", value);

        // value = strtok(NULL, ",");
        // printf("2) %s\n", value);
        // strncpy(&suffix.id.section, value, 1);
        // strncpy(&suffix.id.index, &value[1], 5);
        // strncpy(&suffix.id.type, &value[6], 2);
        // strncpy(&suffix.id.is_exception, &value[7], 1);
        // strncpy(&suffix.id.nlabels, &value[8], 1);

        // printf("");
        // // printf("%c%s%s%c%c\n", suffix.id.section, suffix.id.index, suffix.id.type, suffix.id.is_exception, suffix.id.nlabels);

        ++line_number;
    }

    // for (size_t i = 0; i < lines; i++) {
    //     printf("%-10zu\t%s\n", i, suffixes[i].suffix);
    // }
    
    *suffixes_ptr = suffixes;
    *nsuffixes = lines;
}