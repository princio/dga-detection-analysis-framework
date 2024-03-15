#include "psltrie.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define CHAR_TO_INDEX(c) ((int) c - (int) '!')
#define INDEX_TO_CHAR(i) (char) (i + (int) '!')

#define ROOT_DEPTH -1
#define IS_ROOT(crawl) ((crawl)->depth == ROOT_DEPTH)


char PSLT_GROUP_STR[N_PSLTSUFFIXGROUP][30] = {
    "tld",
    "icann",
    "private"
};

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
            printf("Something is wrong: %ld > %zu [str len]\n%s\n", cursor_end, strlen(row), row);
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
            suffix->index = (size_t) atoi(field);
        }

        { // suffix
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);
            memcpy(suffix->suffix, field, strlen(field));
            suffix->suffix[strlen(field)] = '\0';
        }

        { // group
            cursor = _pslt_csv_parse_field(buffer, cursor, MAXFIELDSIZE, field);

            if(strcmp(field, "tld") == 0) {
                suffix->group = PSLT_GROUP_TLD;
            } else
            if(strcmp(field, "icann") == 0) {
                suffix->group = PSLT_GROUP_ICANN;
            } else
            if(strcmp(field, "private") == 0) {
                suffix->group = PSLT_GROUP_PRIVATE;
            }
        }

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

    printf("[info] Inserting suffix: '%s'\n", suffix->suffix);
    
    for (level = 0; level < length; level++) {
        index = CHAR_TO_INDEX(inverted[level]);

        if (index > PSLT_DOMAIN_ALPHABET_SIZE) {
            printf("[warn]: char2index oversized skipping suffix %s\n", suffix->suffix);
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
        printf("Compacting: ");
        printf("%s & %s\n", crawl->parent->path, crawl->path);
            

        PSLTDomainSuffix tmp;
        PSLTNode* to_remove;
        PSLTNode* substitutor;

        to_remove = crawl->parent;
        substitutor = crawl;

        printf("from\n\t%p[%d]->%p[%d]->%p\n",
            (void*) to_remove->parent,
            to_remove->abc_index,
            (void*) to_remove,
            substitutor->abc_index,
            (void*) substitutor);

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

        printf("to\n\t%p[%d]->(%p == %p)\n\n",
            (void*) to_remove->parent,
            to_remove->abc_index,
            (void*) to_remove->parent->children[to_remove->abc_index],
            (void*) substitutor);
    }
}

void _pslt_trie_compact_merge(PSLTNode* parent, PSLTNode* child) {
    assert(parent && child);
    assert(parent->nbranches == 1);

    parent->path[strlen(parent->path)] = INDEX_TO_CHAR(child->abc_index);

    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (parent->children[i]) { if (parent->children[i] != child) { printf("%ld) noooo\n", i); } }
        
        parent->children[i] = child->children[i];
        if (parent->children[i]) {
            parent->children[i]->parent = parent;
        }
    }

    parent->suffix = child->suffix;
    if (child->nchildren == 0) {
        parent->nchildren = 0;
    }

    parent->nbranches = child->nbranches;
    parent->children[child->abc_index] = NULL;

    free(child);
}

void _pslt_trie_compact(PSLTNode* current) {
    if (!current) {
        printf("END\n");
    }

    printf("\n");
    printf("current: path      %s\n", current->path);
    printf("         pointer   %p\n", (void*) current);
    printf("         branches  %ld\n", current->nbranches);

    if (current->depth >= 0 && current->nbranches == 1) {
        size_t i;
        for (i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
            if (current->children[i]) break;
        }

        printf("tomerge: path      %s\n", current->children[i]->path);
        printf("         pointer   %p\n", (void*) current->children[i]);
        printf("         branches  %ld\n", current->children[i]->nbranches);

        _pslt_trie_compact_merge(current, current->children[i]);

        printf("merged:  path      %s\n", current->path);
        printf("         pointer   %p\n", (void*) current);
        printf("         branches  %ld\n", current->nbranches);
    }

    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (current->children[i]) {
            _pslt_trie_compact(current->children[i]);
        }
    }
}

int _pslt_trie_load_build(PSLTSuffixes suffixes, PSLTNode** root) {
    *root = _trie_new_node(0, NULL);
 
    for (size_t i = 0; i < suffixes.n; i++) {

        // if (suffixes._[i].is_punycode) {
        //     continue;
        // }

        _trie_insert(*root, &suffixes._[i]);
    }

    _pslt_trie_compact_2(*root);
    
    return 0;
}

PSLT* pslt_trie_load(char suffixlistpath[PATH_MAX]) {
    PSLT* pslt;
    int error;

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

void _pslt_trie_print(PSLTNode* crawl, int n_indent) {
    if (!crawl) {
        printf("END\n");
        return;
    }

    if (IS_ROOT(crawl)) {
        printf("\n\n([char]|[index]) b-[branches] \'[key]\' \'[path]\' [suffix]\n");

    }

    int depth = crawl->depth + 1;
    char indent[1000];
    sprintf(indent, "%*s", n_indent * 4, " ");

    printf("%s(%c|%d) b%-2ld \'%s\' \'%s\' %s\n", indent, INDEX_TO_CHAR(crawl->abc_index), crawl->abc_index, crawl->nbranches, crawl->key, crawl->path, crawl->suffix ? crawl->suffix->suffix : "-");

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
}

int _pslt_domain_suffixes(PSLT* pslt, PSLTObject* obj) {

    if (!pslt) {
        fprintf(stderr, "PSLT is NULL\n");
        return 1;
    }

    struct PSLTNode *crawl;
    size_t level;
    size_t length;
    PSLTDomain inverted;
    
    PSLT_FOR_GROUP(g) {
        obj->suffixes[g] = NULL;
    }

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
        if (crawl->suffix) { // } && (level + 1 == strlen(inverted) || inverted[level + 1] == '.')) {
            if (obj->suffixes[crawl->suffix->group]) {
                printf("Warning: already set fro group %s", PSLT_GROUP_STR[crawl->suffix->group]);
            }
            obj->suffixes[crawl->suffix->group] = crawl;
            // if (first_label) {
            //     obj->suffixes.tld = crawl;
            // }
            // if (crawl->suffix->section == PSLT_SUFFIXSECTION_PRIVATEDOMAIN) {
            //     obj->suffixes.private = crawl;
            // } else {
            //     obj->suffixes.icann = crawl;
            // }
        }
    }

    obj->searched = 1;

    return 0;
}

int _pslt_domain_remove_suffixes(PSLT* pslt, PSLTObject* obj) {
    PSLTSuffix* suffixes[3];
    PSLTSuffix* prev;

    if (!obj->searched) {
        _pslt_domain_suffixes(pslt, obj);
    }

    prev = NULL;
    PSLT_FOR_GROUP(g) {
        suffixes[g] = obj->suffixes[g] ? obj->suffixes[g]->suffix : prev;
        prev = suffixes[g];
    }

    int a = 0;
    PSLT_FOR_GROUP(g) {
        if (suffixes[g]) {
            const size_t l = strlen(obj->domain) - strlen(suffixes[g]->suffix);
            strcpy(obj->suffixless[g], obj->domain);
            obj->suffixless[g][l-1] = '\0';
        } else {
            printf("no suffix for %ld: %s\n", g, obj->domain);
            strcpy(obj->suffixless[g], obj->domain);
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
    
    PSLT_FOR_GROUP(g) {
        if (object->suffixes[g]) {
            biggest = object->suffixes[g]->suffix->suffix;
        }
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
            fprintf(fp_write, "%s,%s", object.domain, object.basedomain);

            PSLT_FOR_GROUP(g) {
                fprintf(fp_write, ",%s", object.suffixes[g] ? object.suffixes[g]->suffix->suffix : "");
            }

            PSLT_FOR_GROUP(g) {
                fprintf(fp_write, ",%s", object.suffixless[g]);
            }

            fprintf(fp_write, "\n");
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