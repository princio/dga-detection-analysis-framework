#include "psltrie.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#define CHAR_TO_INDEX(c) ((int) c - (int) '!')
#define INDEX_TO_CHAR(i) (char) (i + (int) '!')

int _pslt_csv_parse_field(char *row, const size_t cursor_start, const size_t max_field_size, char field[max_field_size]) {
    size_t cursor_end;

    {
        char * location_end = strchr(row + cursor_start, ',');

        if (location_end) {
            cursor_end = location_end - row;
        } else {
            cursor_end = strlen(row);
        }

        if (cursor_end > strlen(row)) {
            printf("Something is wrong: %ld > %zu [str len]", cursor_end, strlen(row));
            cursor_end = strlen(row);
        }
    }

    if (field) {
        memset(field, 0, max_field_size);
        memcpy(field, &row[cursor_start], cursor_end - cursor_start);
    }

    return cursor_end + 1;
}

int pslt_domain_count_labels(PSLTDomain domain) {
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

int pslt_domain_labels(PSLTDomain domain, PSLTDomainLabels labels) {
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

int pslt_domain_invert(PSLTDomain domain, PSLTDomain inverted) {
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

int _pslt_suffixes_parse(char filepath[PATH_MAX], PSLTSuffixes* suffixes) {
    const size_t MAXFIELDSIZE = 500;
    const size_t MAXROWSIZE = PSLT_LIST_ROW_FIELDS_NUMBER * MAXFIELDSIZE;

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
    while(fgets(buffer, PSLT_LIST_ROW_FIELDS_NUMBER * sizeof(field), fp)) {
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
        PSLTSuffix* suffix = &suffixes->_[line_number];
        int cursor = 0;

        {
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->index = (size_t) atoi(field);
        }

        { // code
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            memcpy(&suffix->code.section, field, strlen(field));
        }

        { // suffix
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            memcpy(suffix->suffix, field, strlen(field));
            suffix->suffix[strlen(field)] = '\0';
        }

        { // tld
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
        }

        { // punycode
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->is_punycode = strlen(field) > 0;
        }

        { // type
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
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
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
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
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
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

        { // newGLTD
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->is_newGLTD = 0 == strcmp(field, "True");
        }

        { // exception
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->is_exception = 0 == strcmp(field, "True");
        }

        { // isprivate
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->is_private = 0 != strcmp(field, "icann");
        }

        ++line_number;

        #undef PARSE_FIELD
    }

    suffixes->n = line_number;

    fclose(fp);

    (void)(unused);

    return PSLT_ERROR_NO;
}

PSLTNode* _trie_new_node() {
    PSLTNode *node = NULL;
 
    node = (PSLTNode*) malloc(sizeof(struct PSLTNode));
 
    if (node) {
        unsigned int i;
 
        node->isEndOfSuffix = 0;
        node->suffix = NULL;

        for (i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; ++i) node->children[i] = NULL;
    }
 
    return node;
}
 
void _trie_insert(PSLTNode* root, PSLTSuffix* suffix) {
    
    int level;
    int length = strlen(suffix->suffix);
    unsigned int index;
 
    PSLTNode* crawl = root;

    char inverted[strlen(suffix->suffix) + 1];
    pslt_domain_invert(suffix->suffix, inverted);
    
    for (level = 0; level < length; level++)
    {
        index = CHAR_TO_INDEX(inverted[level]);

        if (index > PSLT_DOMAIN_ALPHABET_SIZE) continue;

        if (!crawl->children[index]) {
            crawl->children[index] = _trie_new_node();
        }
 
        crawl = crawl->children[index];
    }
 
    crawl->suffix = suffix;
    crawl->isEndOfSuffix = 1;
}

PSLTError _pslt_trie_load_build(PSLTSuffixes suffixes, PSLTNode** root) {
    *root = _trie_new_node();
 
    for (size_t i = 0; i < suffixes.n; i++) {

        if (suffixes._[i].is_punycode) {
            continue;
        }

        _trie_insert(*root, &suffixes._[i]);
    }
    
    return PSLT_ERROR_NO;
}

PSLT* pslt_trie_load(char suffixlistpath[PATH_MAX]) {
    PSLT* pslt;
    PSLTError error;

    pslt = calloc(1, sizeof(PSLT));

    error = _pslt_suffixes_parse(suffixlistpath, &pslt->suffixes);

    if (error) {
        fprintf(stderr, "Error during opening file[%s]: %s", suffixlistpath, strerror(errno));
        return NULL;
    }

    _pslt_trie_load_build(pslt->suffixes, &pslt->trie);

    if (pslt->trie == NULL) {
        fprintf(stderr, "Impossible to load the prefix tree.\n");
        return NULL;
    }

    return pslt;
}

void _pslt_trie_print(PSLTNode* crawl, char path[1000], int level) {
    if (!crawl) {
        printf("END\n");
        return;
    }

    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (crawl->children[i]) {
            path[level] = INDEX_TO_CHAR(i);
            path[level + 1] = '\0';
            printf("%*s%2d/%s\n", 4*level, " ", level, path);
            if (crawl->children[i]->isEndOfSuffix) {
                printf("%*s\t%s\n", 4*level + 2, " ", crawl->children[i]->suffix->suffix);
            } else {
                _pslt_trie_print(crawl->children[i], path, level + 1);
            }
        }
    }
}

void pslt_trie_print(PSLT* pslt) {
    char path[1000];
    _pslt_trie_print(pslt->trie, path, 0);
}

void _pslt_free_trie(PSLTNode* crawl) {
    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (crawl->children[i] != NULL) {
            _pslt_free_trie(crawl->children[i]);
        }
    }
    free(crawl);
}

void pslt_trie_free(PSLT* pslt) {
    _pslt_free_trie(pslt->trie);
    free(pslt->suffixes._);
    free(pslt);
}

int _pslt_domain_suffixes(PSLT* pslt, PSLTObject* obj) {

    if (!pslt) {
        fprintf(stderr, "PSLT is NULL\n");
        return 1;
    }

    memset(&obj->suffixes, 0, sizeof(PSLTTriesSuffixes));

    struct PSLTNode *crawl;
    size_t level;
    size_t length;
    PSLTDomain inverted;
    
    obj->suffixes.tld = NULL;
    obj->suffixes.icann = NULL;
    obj->suffixes.private = NULL;

    crawl = pslt->trie;
    length = strlen(obj->domain);

    pslt_domain_invert(obj->domain, inverted);
 
    size_t first_label = 1;
    for (level = 0; level < length; level++) {

        int index = CHAR_TO_INDEX(inverted[level]);

        if (first_label && level > 0 && inverted[level - 1] == '.') {
            first_label = 0;
        }

        if (index > PSLT_DOMAIN_ALPHABET_SIZE || index < 0) {
            continue;
        }
 
        if (!crawl->children[index]) {
            break;
        }
 
        crawl = crawl->children[index];
        if (crawl->suffix && (level + 1 == strlen(inverted) || inverted[level + 1] == '.')) {
            if (first_label) {
                obj->suffixes.tld = crawl;
            }
            if (crawl->suffix->section == PSLT_SUFFIXSECTION_PRIVATEDOMAIN) {
                obj->suffixes.private = crawl;
            } else {
                obj->suffixes.icann = crawl;
            }
        }
    }

    obj->searched = 1;

    return 0;
}

int _pslt_domain_remove_suffixes(PSLT* pslt, PSLTObject* obj) {
    PSLTSuffix* suffixes[3];

    if (!obj->searched) {
        _pslt_domain_suffixes(pslt, obj);
    }

    suffixes[0] = obj->suffixes.tld ? obj->suffixes.tld->suffix : NULL;
    suffixes[1] = obj->suffixes.icann ? obj->suffixes.icann->suffix : suffixes[0];
    suffixes[2] = obj->suffixes.private ? obj->suffixes.private->suffix : suffixes[1];

    char* domains_processed[3] = {
        obj->wo_suffixes.tld,
        obj->wo_suffixes.icann,
        obj->wo_suffixes.private
    };
    int a = 0;
    for (size_t s = 0; s < 3; s++) {
        if (suffixes[s]) {
            const size_t l = strlen(obj->domain) - strlen(suffixes[s]->suffix);
            strcpy(domains_processed[s], obj->domain);
            domains_processed[s][l-1] = '\0';
        } else {
            printf("no suffix for %d: %s\n", s, obj->domain);
            strcpy(domains_processed[s], obj->domain);
            a=1;
        }
    }
    if (a) 
    printf("\n");

    return 0;
}

int _pslt_domain_basedomain(PSLT* pslt, PSLTObject* object) {
    char* biggest = NULL;

    if (!object->searched) {
        _pslt_domain_suffixes(pslt, object);
    }

    if (strlen(object->domain) == 0) {
        return 1;
    }
    
    if (object->suffixes.private) {
        biggest = object->suffixes.private->suffix->suffix;
    } else if (object->suffixes.icann) {
        biggest = object->suffixes.icann->suffix->suffix;
    } else if (object->suffixes.tld) {
        biggest = object->suffixes.tld->suffix->suffix;
    }

    strcpy(object->basedomain, object->domain);

    if (biggest == NULL) {
        return 0;
    }

    object->basedomain[strlen(object->domain) - strlen(biggest) - 1] = '\0';

    // PSLTDomain tmp;
    // strcpy(tmp, object->domain);

    // char* pos = strstr(tmp, biggest);
    // if (pos > tmp) {
    //     --pos;
    // }
    // *pos = '\0';

    // char *pos2 = strrchr(tmp, '.');
    // if (pos2) {
    //     strcpy(object->basedomain, object->domain + (pos2 - tmp + 1));
    // } else {
    //     strcpy(object->basedomain, object->domain);
    // }

    return 0;
}

int pslt_domain_run(PSLT* pslt, PSLTObject* obj) {
    if (!pslt) {
        fprintf(stderr, "PSLT is null");
        return 1;
    }
    if (!obj) {
        fprintf(stderr, "PSLT object is null");
        return 1;
    }

    _pslt_domain_suffixes(pslt, obj);
    _pslt_domain_basedomain(pslt, obj);
    _pslt_domain_remove_suffixes(pslt, obj);

    return 0;
}

int pslt_csv(PSLT* pslt, int dn_col, char csv_in_path[PATH_MAX], char csv_out_path[PATH_MAX]) {
    char* unused;

    FILE* fp;
    FILE* fp_write;

    fp = fopen(csv_in_path, "r");
    if (!fp){
        fprintf(stderr, "Impossible to open list file\n");
        exit(1);
    }

    fp_write = fopen(csv_out_path, "w");
    if (!fp_write){
        fprintf(stderr, "Impossible to create output file\n");
        exit(1);
    }

    const int MAXROWSIZE = 10000;
    char buffer[MAXROWSIZE];

    unused = fgets(buffer, MAXROWSIZE, fp); // skip the header (one line)

    fprintf(fp_write, "dn,bdn,tld,icann,private,dn_tld,dn_icann,dn_private\n");

    int cursor = 0;
    size_t n = 0;
    PSLTObject object;
    while(fgets(buffer, MAXROWSIZE, fp)) {
        cursor = 0;
        memset(&object, 0, sizeof(PSLTObject));

        for (int i = 0; i < dn_col; i++) {
            cursor = _pslt_csv_parse_field(buffer, cursor, 0, NULL);
        }

        cursor = _pslt_csv_parse_field(buffer, cursor, sizeof(PSLTDomain), object.domain);

        if (!pslt_domain_run(pslt, &object)) {

            fprintf(fp_write, "%s,%s,", object.domain, object.basedomain);

            fprintf(fp_write, "%s,", object.suffixes.tld ? object.suffixes.tld->suffix->suffix : "");
            fprintf(fp_write, "%s,", object.suffixes.icann ? object.suffixes.icann->suffix->suffix : "");
            fprintf(fp_write, "%s,",  object.suffixes.private ? object.suffixes.private->suffix->suffix : "");

            fprintf(fp_write, "%s,", object.wo_suffixes.tld);
            fprintf(fp_write, "%s,", object.wo_suffixes.icann);
            fprintf(fp_write, "%s\n",  object.wo_suffixes.private);
        }

        if (n && n % 1000 == 0) {
            fflush(fp_write);
        }

        ++n;
    }

    fclose(fp);
    fclose(fp_write);

    (void)(unused);

    return 0;
}