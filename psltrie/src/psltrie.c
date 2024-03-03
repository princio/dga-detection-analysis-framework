#include "psltrie.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#define CHAR_TO_INDEX(c) ((int) c - (int) '!')

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

void _pslt_free_trie(PSLTNode* crawl) {
    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (crawl->children[i] != NULL) {
            _pslt_free_trie(crawl->children[i]);
        }
    }
    free(crawl);
}

int pslt_count_domain_labels(PSLTDomainFull domain) {
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

int pslt_domain_labels(PSLTDomainFull domain, PSLTDomainLabels labels) {
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

int pslt_domain_invert(PSLTDomainFull domain, PSLTDomainFull inverted) {
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

PSLTError plst_build(PSLTSuffixes suffixes, PSLTNode** root) {
    *root = _trie_new_node();
 
    for (size_t i = 0; i < suffixes.n; i++) {

        if (suffixes._[i].is_punycode) {
            continue;
        }

        _trie_insert(*root, &suffixes._[i]);
    }
    
    // _search(root, "aps.la.ps.oa.blokko.co.uk2");
    // _search(root, "aps.la.ps.oa.blokko.3utilities.com");
    // _search(root, "aps.la.ps.oa.blokko.localzone.xyz");
    // _search(root, "aps.la.ps.oa.blokko.nl-ams-1.baremetal.scw.cloud");
    // _search(root, "aps.la.ps.oa.blokko.dh.bytemark.co.uk");

    return PSLT_ERROR_NO;
}

PSLTSuffixSearchResult pslt_search(PSLT* pslt, PSLTDomainFull domain) {
    struct PSLTNode *crawl;
    size_t level;
    size_t length;
    PSLTDomainFull inverted;
    
    PSLTSuffixSearchResult result;
    result.tld = NULL;
    result.icann = NULL;
    result.private = NULL;

    crawl = pslt->trie;
    length = strlen(domain);

    pslt_domain_invert(domain, inverted);
 
    size_t first_label = 1;
    for (level = 0; level < length; level++) {

        int index = CHAR_TO_INDEX(inverted[level]);

        if (first_label && level > 0 && inverted[level - 1] == '.') {
            first_label = 0;
        }

        if (index > PSLT_DOMAIN_ALPHABET_SIZE || index < 0) continue;
 
        if (!crawl->children[index]) {
            break;
        }
 
        crawl = crawl->children[index];
        if (crawl->suffix && (level + 1 == strlen(inverted) || inverted[level + 1] == '.')) {
            if (first_label) {
                result.tld = crawl;
            }
            if (crawl->suffix->section == PSLT_SUFFIXSECTION_PRIVATEDOMAIN) {
                result.private = crawl;
            } else {
                result.icann = crawl;
            }
        }
    }

    return result;
}

PSLT* pslt_load(char suffixlistpath[PATH_MAX]) {
    PSLT* pslt;
    PSLTError error;

    pslt = calloc(1, sizeof(PSLT));

    error = pslt_suffixes_parse(suffixlistpath, &pslt->suffixes);

    if (error) {
        fprintf(stderr, "Error during opening file: %s", strerror(errno));
        return NULL;
    }

    plst_build(pslt->suffixes, &pslt->trie);

    if (pslt->trie == NULL) {
        fprintf(stderr, "Impossible to load the prefix tree.");
        return NULL;
    }

    return pslt;
}

void pslt_free(PSLT* pslt) {
    _pslt_free_trie(pslt->trie);
    free(pslt->suffixes._);
    free(pslt);
}