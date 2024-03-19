#include "psltrie.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CHAR_TO_INDEX(c) ((int) c - (int) '!')
#define INDEX_TO_CHAR(i) (char) (i + (int) '!')

#define ROOT_DEPTH -1
#define IS_ROOT(crawl) ((crawl)->depth == ROOT_DEPTH)

#ifdef DEBUG
#define PRINTF(F, ...) printf(F, ##__VA_ARGS__ )
#else
#define PRINTF(F, ...) { }
#endif


char PSLT_GROUP_STR[N_PSLTSUFFIXGROUP][30] = {
    "tld",
    "icann",
    "private"
};

FILE* pslt_logger_file = NULL;
enum PSLTLogLevel pslt_logger_level = PSLT_LOG_LEVEL_NONE;

int _pslt_csv_parse_field(char *row, const size_t cursor_start, const size_t max_field_size, char field[max_field_size]) {
    size_t cursor_end;

    {
        char * location_end = strchr(row + cursor_start, ',');

        if (location_end) {
            cursor_end = location_end - row;
        } else {
            cursor_end = strlen(row);
            if (row[cursor_end - 1] == '\n') {
                --cursor_end;
            }
        }

        if (cursor_end > strlen(row)) {
            LOG_ERROR("something is wrong: %ld > %zu [str len]\n%s\n", cursor_end, strlen(row), row);
            cursor_end = strlen(row);
        }
    }

    if (field) {
        memset(field, 0, max_field_size);
        if (cursor_end - cursor_start > max_field_size) {
            LOG_ERROR("Something went wrong: %ld\n", cursor_end - cursor_start);
        } else {
            memcpy(field, &row[cursor_start], cursor_end - cursor_start);
        }
    }

    return cursor_end + 1;
}

int pslt_domain_count_labels(PSLTDomain domain) {
    const char* begin = domain;
    const char* end = &domain[strlen(domain) - 1];
    int nlabels = 0;

    do {
        nlabels++;
        begin = strchr(begin + 1, '.');
    } while (begin && begin + 1 < end);

    return nlabels;
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

    PSLTDomain tmp;
    strcpy(tmp, domain);

    char* dot;
    char* last_dot = &tmp[strlen(tmp)];
    while ((dot = strrchr(tmp, '.'))) {
        strncat(inverted, dot + 1, last_dot - dot);
        inverted[strlen(inverted)] = '.';
        dot[0] = '\0';
        last_dot = dot;
    }
    strncat(inverted, tmp, last_dot - tmp);
    // for(int i = len - 1; i >=0; ) {
    //     int dot_position;
    //     int domain_cursor;
    //     int inv_cursor;
    //     int label_len;
        
    //     dot_position = i - 1;
    //     while(domain[dot_position] != '.' && dot_position > 0) { --dot_position; }

    //     domain_cursor = dot_position + (dot_position > 0);
    //     inv_cursor = (len - i - 1);
    //     label_len = i - dot_position + (dot_position== 0);

    //     strncpy(inverted + inv_cursor, domain + domain_cursor, label_len);

    //     if (dot_position > 0)
    //         inverted[inv_cursor + label_len] = '.';

    //     i = dot_position - 1;
    // }

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
        return 1;
    }

    lines = 0;
    while(fgets(buffer, PSLT_LIST_ROW_FIELDS_NUMBER * sizeof(field), fp)) {
        ++lines;
    }
    --lines;

    suffixes->_ = calloc(lines, sizeof(PSLTSuffix));

    // suffixes->_[0].index = 5; // TODO: CHECK
    // suffixes->_[10].index = 15; // TODO: CHECK

    rewind(fp);

    line_number = 0;
    unused = fgets(buffer, MAXROWSIZE, fp); // skip the header (1st line)
    unused = fgets(buffer, MAXROWSIZE, fp); // skip the header (2nd line)
    while(fgets(buffer, MAXROWSIZE, fp)) {
        PSLTSuffix* suffix = &suffixes->_[line_number];
        int cursor = 0;

        { // index
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->index = (size_t) atoll(field);
        }

        { // suffix
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            memcpy(suffix->suffix, field, strlen(field));
            suffix->suffix[strlen(field)] = '\0';
        }

        { // group
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);

            for (suffix->group = PSLT_GROUP_TLD; suffix->group < N_PSLTSUFFIXGROUP; suffix->group++) {
                if (!strcmp(field, PSLT_GROUP_STR[suffix->group])) {
                    break;
                }
            }
        }

        { // is_ascii
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            suffix->is_ascii = atoi(field);
        }

        pslt_domain_invert(suffix->suffix, suffix->inverted);

        ++line_number;
    }

    suffixes->n = line_number;

    fclose(fp);

    (void)(unused);

    return 0;
}

PSLTNode* _trie_new_node(int abc_index, PSLTNode* parent) {
    PSLTNode* node;
 
    node = (PSLTNode*) calloc(1, sizeof(PSLTNode));
 
    if (node) {
        PSLTNode* tmp;

        if (!parent) {
            node->depth = ROOT_DEPTH;
        } else {
            node->nbranches = 0;
            node->nchildren = 0;
            node->depth = parent->depth + 1;
            node->parent = parent;
            node->abc_index = abc_index;

            node->path[0] = INDEX_TO_CHAR(abc_index);
            node->key[0] = INDEX_TO_CHAR(abc_index);

            if (strlen(node->path) + strlen(node->parent->path) < sizeof(PSLTDomainSuffix)) {
                PSLTDomainSuffix tmp;
                strcpy(tmp, node->path);
                strcpy(node->path, node->parent->path);
                strcat(node->path, tmp);
            }
        }

        tmp = node;
        while ((tmp = tmp->parent)) {
            tmp->nchildren++;
        }
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

    LOG_INFO("inserting [%s] suffix: '%s' [inv][%s]", suffix->is_ascii ? "ascii" : "utf8", suffix->suffix, inverted);
    
    for (level = 0; level < length; level++) {
        index = CHAR_TO_INDEX(inverted[level]);

        if (index > PSLT_DOMAIN_ALPHABET_SIZE) {
            LOG_WARN("char2index oversized (%d|%c) skipping suffix %s", index, inverted[level], suffix->suffix);
            return;
        }

        if (!crawl->children[index]) {
            crawl->children[index] = _trie_new_node(index, crawl);
            crawl->nbranches++;
        }


        crawl = crawl->children[index];
    }
 
    crawl->suffix = suffix;
}

void _pslt_trie_compact_2(PSLTNode* crawl) {
    if (crawl->nchildren > 0) {
        for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
            if (crawl->children[i]) {
                _pslt_trie_compact_2(crawl->children[i]);
            }
        }
    }

    if (crawl->compacted) {
        PSLTNode* tmp = crawl->compacted;
        free(crawl);
        crawl = tmp;
    }

    if (IS_ROOT(crawl) || IS_ROOT(crawl->parent)) {
        return;
    }

    if (crawl->parent->nbranches == 1 && crawl->parent->suffix == NULL) {
        // LOG_TRACE("Compacting %s & %s:\t", crawl->parent->path, crawl->path);
            
        PSLTDomainSuffix tmp;
        PSLTNode* to_remove;
        PSLTNode* substitutor;

        to_remove = crawl->parent;
        substitutor = crawl;

        // LOG_TRACE("from\t%p[%d]->%p[%d]->%p\t",
        //     (void*) to_remove->parent,
        //     to_remove->abc_index,
        //     (void*) to_remove,
        //     substitutor->abc_index,
        //     (void*) substitutor);

        substitutor->parent = to_remove->parent;

        to_remove->parent->children[to_remove->abc_index] = substitutor;

        assert(strlen(substitutor->path) + strlen(to_remove->path) < sizeof(PSLTDomainSuffix));
        strcpy(tmp, substitutor->key);
        strcpy(substitutor->path, to_remove->path);
        strcat(substitutor->path, tmp);

        assert(strlen(substitutor->key) + strlen(to_remove->key) < sizeof(PSLTDomainSuffix));
        strcpy(tmp, substitutor->key);
        strcpy(substitutor->key, to_remove->key);
        strcat(substitutor->key, tmp);

        to_remove->compacted = substitutor;

        // LOG_TRACE("to\t%p[%d]->(%p == %p)\n",
        //     (void*) to_remove->parent,
        //     to_remove->abc_index,
        //     (void*) to_remove->parent->children[to_remove->abc_index],
        //     (void*) substitutor);
    }
}

int _pslt_trie_load_build(PSLTSuffixes suffixes, PSLTNode** root) {
    *root = _trie_new_node(0, NULL);
 
    for (size_t i = 0; i < suffixes.n; i++) {
        if (!suffixes._[i].is_ascii) {
            LOG_TRACE("is not ascii: %s\n", suffixes._[i].suffix);
        }
        if (suffixes._[i].suffix[0] == '*') {
            LOG_TRACE("skipping asterisk");
        } else {
            _trie_insert(*root, &suffixes._[i]);
        }
    }

    _pslt_trie_compact_2(*root);
    
    return 0;
}

PSLT* pslt_trie_load(char suffixlistpath[PATH_MAX]) {
    PSLT* pslt;
    int error;

    if (!pslt_logger_file) {
        pslt_logger_file = fopen("/tmp/psl_list.log", "w");
    }

    pslt = calloc(1, sizeof(PSLT));

    error = _pslt_suffixes_parse(suffixlistpath, &pslt->suffixes);

    if (error) {
        LOG_ERROR("failed to open file '%s': %s", suffixlistpath, strerror(errno));
        return NULL;
    }

    _pslt_trie_load_build(pslt->suffixes, &pslt->trie);

    if (pslt->trie == NULL) {
        LOG_ERROR("impossible to load the prefix trie.");
        return NULL;
    }

    return pslt;
}

void _pslt_trie_print(PSLTNode* crawl, int n_indent) {
    if (!crawl) {
        printf("END\n");
        return;
    }

    if (IS_ROOT(crawl)) {
        printf("\n\n([char]|[index]) b-[branches] \'[key]\' \'[path]\' [suffix]\n");

    }

    char indent[1000];
    sprintf(indent, "%*s", n_indent * 4, " ");

    printf("%s(%c|%d) ",
        indent,
        INDEX_TO_CHAR(crawl->abc_index),
        crawl->abc_index);
    printf("b%-2ld ", crawl->nbranches);
    printf("k'%s' ", crawl->key);
    printf("p'%s' ", crawl->path);
    if (crawl->suffix) {
        printf("'%s' ", crawl->suffix ? crawl->suffix->suffix : "-");
        printf("[%s]", crawl->suffix->is_ascii ? "ascii" : "utf-8");
    }
    printf("\n");

    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (crawl->children[i]) {
            _pslt_trie_print(crawl->children[i], n_indent + 1);
        }
    }
}

void pslt_trie_print(PSLT* pslt) {
    _pslt_trie_print(pslt->trie, 0);
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

    if (!pslt_logger_file) {
        fclose(pslt_logger_file);
    }
}

int _pslt_domain_suffixes_search_found(PSLTDomain inverted, PSLTNode* crawl, PSLTObject* obj) {
    assert(crawl->suffix);

    int rcode;
    
    rcode = 0;

    int const correspond = !strncmp(inverted, crawl->suffix->inverted, strlen(crawl->suffix->suffix));

    if (correspond) {
        if (obj->suffixes[crawl->suffix->group]) {
            LOG_WARN("suffix already set for group '%s': replacing '%s' with '%s'", PSLT_GROUP_STR[crawl->suffix->group], obj->suffixes[crawl->suffix->group]->suffix->suffix, crawl->suffix->suffix);
        } else {
            LOG_INFO("suffix found '%s' of group %s for '%s'[inv::%s]", crawl->suffix->suffix, PSLT_GROUP_STR[crawl->suffix->group], obj->domain, inverted);
            rcode = 1;
        }
        obj->suffixes[crawl->suffix->group] = crawl;
    }

    return rcode;
}

int _pslt_domain_suffixes_search(PSLT* pslt, PSLTObject* obj) {
    struct PSLTNode *crawl;
    PSLTDomain inverted;
    size_t cursor;
    int suffixes_found;
    
    PSLT_FOR_GROUP(g) {
        obj->suffixes[g] = NULL;
    }

    crawl = pslt->trie;

    pslt_domain_invert(obj->domain, inverted);

    { // remove last label from inverted
        char *last_dot = strrchr(inverted, '.');
        (last_dot + 1)[0] = '\0';
    }

    suffixes_found = 0;
    cursor = 0;
    while(cursor < strlen(inverted)) {
        int index;
    
        if (strncmp(crawl->path, inverted, cursor)) {
            char tmp[cursor+1];
            strncpy(tmp, inverted, cursor);
            tmp[cursor] = '\0';
            LOG_ERROR("path and current search string not correspond: %s <> %s", crawl->path, tmp);
            break;
        }
        if (crawl->nbranches == 0) {
            LOG_INFO("no further suffixes, search is over for domain '%s'", inverted);
            LOG_INFO("                                                %*c", (int) cursor, '^');
            break;
        }

        index = CHAR_TO_INDEX(inverted[cursor]);

        if (index > PSLT_DOMAIN_ALPHABET_SIZE || index < 0) {
            LOG_WARN("abc-index out of range, suffix not found for domain '%s'", obj->domain);
            LOG_WARN("                                                     %*c", (int) cursor, '^');
            break;
        }
        if (!crawl->children[index]) {
            LOG_INFO("abc-index out of range, search over for domain '%s'", inverted);
            LOG_INFO("                                                %*c", (int) cursor, '^');
            break;
        }

        LOG_TRACE("current index '%c' with path: '%s'", INDEX_TO_CHAR(index), crawl->path);

        crawl = crawl->children[index];

        cursor = strlen(crawl->path);

        if (crawl->suffix) {
            LOG_TRACE("with suffix: '%s'", crawl->suffix->suffix);
            suffixes_found += _pslt_domain_suffixes_search_found(inverted, crawl, obj);
        }
    }
    
    obj->searched = 1;

    return suffixes_found;
}

int _pslt_domain_remove_suffixes(PSLT* pslt, PSLTObject* obj) {
    PSLTSuffix* suffixes[3];
    PSLTSuffix* prev;

    if (!obj->searched) {
        _pslt_domain_suffixes_search(pslt, obj);
    }

    prev = NULL;
    PSLT_FOR_GROUP(g) {
        suffixes[g] = obj->suffixes[g] ? obj->suffixes[g]->suffix : prev;
        prev = suffixes[g];
    }

    PSLT_FOR_GROUP(g) {
        if (suffixes[g]) {
            const size_t l = strlen(obj->domain) - strlen(suffixes[g]->suffix);
            strcpy(obj->suffixless[g], obj->domain);
            obj->suffixless[g][l-1] = '\0';
        } else {
            LOG_INFO("no suffix for group %s: '%s'", PSLT_GROUP_STR[g], obj->domain);
            strcpy(obj->suffixless[g], obj->domain);
        }
    }

    return 0;
}

int _pslt_domain_basedomain(PSLT* pslt, PSLTObject* object) {
    if (strlen(object->domain) == 0) {
        LOG_ERROR("domain length is zero.");
        return 1;
    }

    if (!object->searched) {
        _pslt_domain_suffixes_search(pslt, object);
    }
    
    PSLTSuffix* suffix;
    PSLT_FOR_GROUP(g) {
        if (object->suffixes[g]) {
            suffix = object->suffixes[g]->suffix;
        }
    }

    char* dot;
    int domain_labels;
    int suffix_labels;

    char const * domain_end = &object->domain[strlen(object->domain) - 1];
    char const * suffix_end = &suffix->suffix[strlen(suffix->suffix) - 1];

    dot = object->domain;
    domain_labels = 0;
    do {
        domain_labels++;
        dot = strchr(dot + 1, '.');
    } while (dot && dot + 1 < domain_end);

    dot = suffix->suffix;
    suffix_labels = 0;
    do {
        suffix_labels++;
        dot = strchr(dot + 1, '.');
    } while (dot && dot + 1 < suffix_end);

    int skipped_labels = domain_labels - (suffix_labels + 1);

    dot = object->domain;
    if (skipped_labels > 0) {
        dot = object->domain;
        for (int i = 0; i < skipped_labels; i++) {
            if (dot + 1 < domain_end) {
                dot = strchr(dot + 1, '.');
            } else {
                break;
            }
        }
        dot += 1;
    }

    strcpy(object->basedomain, dot);

    return 0;
}

int pslt_domain_run(PSLT* pslt, PSLTObject* obj) {
    int suffixes_found;

    if (!pslt) {
        LOG_WARN("PSLT is null");
        return -1;
    }
    if (!obj) {
        LOG_WARN("PSLT object is null");
        return -1;
    }
    if (obj->domain[0] == '.') {
        LOG_WARN("domain starting with a dot: %s", obj->domain);
        return -1;
    }
    if (obj->domain[strlen(obj->domain) - 1] == '.') {
        LOG_WARN("domain   ending with a dot: %s", obj->domain);
        return -1;
    }
    if (strlen(obj->domain) < 3) {
        LOG_WARN("domain   too short: %s", obj->domain);
        return -1;
    }
    if (pslt_domain_count_labels(obj->domain) <= 1) {
        LOG_WARN("domain   having too many labels: %s", obj->domain);
        return -1;
    }

    for (size_t i = 0; i < strlen(obj->domain); i++) {
        obj->domain[i] = tolower(obj->domain[i]);
    }

    suffixes_found = _pslt_domain_suffixes_search(pslt, obj);
    if (!suffixes_found) {
        strcpy(obj->basedomain, obj->domain);
        PSLT_FOR_GROUP(g) {
            strcpy(obj->suffixless[g], obj->domain);
        }
    } else {
        _pslt_domain_basedomain(pslt, obj);
        _pslt_domain_remove_suffixes(pslt, obj);
    }

    return suffixes_found;
}

int pslt_csv(PSLT* pslt, int dn_col, char csv_in_path[PATH_MAX], char csv_out_path[PATH_MAX]) {
    char* unused;

    FILE* fp;
    FILE* fp_write;

    pslt_logger_level = PSLT_LOG_LEVEL_TRACE;

    fp = fopen(csv_in_path, "r");
    if (!fp){
        fprintf(stderr, "Impossible to open csv file\n");
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

    fprintf(fp_write, "dn,bdn,rcode,tld,icann,private,dn_tld,dn_icann,dn_private\n");

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

        int ret = pslt_domain_run(pslt, &object);

        fprintf(fp_write, "%s,", object.domain);
        fprintf(fp_write, "%s,", object.basedomain);
        fprintf(fp_write, "%d", ret);

        PSLT_FOR_GROUP(g) {
            fprintf(fp_write, ",%s", object.suffixes[g] ? object.suffixes[g]->suffix->suffix : "");
        }

        PSLT_FOR_GROUP(g) {
            fprintf(fp_write, ",%s", object.suffixless[g]);
        }

        fprintf(fp_write, "\n");

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

int pslt_csv_test(PSLT* pslt, char csvtest_path[PATH_MAX]) {
    char* unused;
    const int MAXROWSIZE = sizeof(PSLTDomain) * 5;
    const int MAXFIELDSIZE = sizeof(PSLTDomain);

    FILE *fp, *fpw;
    PSLTObject obj;
    int cursor;
    char buffer[MAXROWSIZE];

    pslt_logger_level = PSLT_LOG_LEVEL_TRACE;

    char field[MAXFIELDSIZE];

    fp = fopen(csvtest_path, "r");
    if (!fp){
        fprintf(stderr, "Impossible to open list file\n");
        exit(1);
    }

    fpw = fopen("/tmp/pslregex.test.log", "w");
    if (!fpw){
        fprintf(stderr, "Impossible to open test file\n");
        exit(1);
    }

    while(fgets(buffer, MAXROWSIZE, fp)) {
        size_t index;
        PSLTDomainSuffix suffix;
        PSLTSuffixGroup group;
        
        cursor = 0;

        { // index
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            index = (size_t) atoll(field);
        }

        { // suffix
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            memcpy(suffix, field, strlen(field));
            suffix[strlen(field)] = '\0';
        }

        { // group
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);

            for (group = PSLT_GROUP_TLD; group < N_PSLTSUFFIXGROUP; group++) {
                if (!strcmp(field, PSLT_GROUP_STR[group])) {
                    break;
                }
            }
        }

        fprintf(fpw, "%ld,", index);
        fprintf(fpw, "%s,", suffix);
        fprintf(fpw, "%s,", PSLT_GROUP_STR[group]);

        for (size_t i = 0; i < 3; i++) {
            memset(&obj, 0, sizeof(PSLTObject));
            cursor = _pslt_csv_parse_field(buffer, cursor, sizeof(PSLTDomain), obj.domain);

            int ret = pslt_domain_run(pslt, &obj);

            switch (ret) {
                case -1:
                    fprintf(fpw, "%s,", "invalid");
                    break;
                case 0:
                    fprintf(fpw, "%s,", "not-found");
                    break;
                default: {
                    if (group >= N_PSLTSUFFIXGROUP) {
                        printf("something is wrong");
                    }
                    if (obj.suffixes[group] && obj.suffixes[group]->suffix) {
                        if (strcmp(suffix, obj.suffixes[group]->suffix->suffix)) {
                            fprintf(fpw, "%s,", "DIS");
                        } else
                        if (!strcmp(suffix, obj.suffixes[group]->suffix->suffix)) {
                            fprintf(fpw, "%s,", "EQ");
                        }
                        fprintf(fpw, "%s,", obj.suffixes[group]->suffix->suffix);
                    } else {
                        for (size_t _group = PSLT_GROUP_TLD; _group < N_PSLTSUFFIXGROUP; _group++) {
                            if (obj.suffixes[_group]) {
                                fprintf(fpw, "OTH::%s,", PSLT_GROUP_STR[_group]);
                                fprintf(fpw, "%s,", obj.suffixes[_group]->suffix->suffix);
                            }
                        }
                    }
                    break;
                }
            }
        }
        fseek(fpw, -1, SEEK_CUR);
        fprintf(fpw, "\n");
        fflush(fpw);
    }

    fclose(fp);
    fclose(fpw);

    (void)(unused);

    return 0;
}