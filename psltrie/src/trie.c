#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "psltrie.h"

#define CHAR_TO_INDEX(c) ((int) c - (int) '!')
 
PSLTNode* _trie_new_node() {
    PSLTNode *node = NULL;
 
    node = (PSLTNode*) malloc(sizeof(struct PSLTNode));
 
    if (node) {
        unsigned int i;
 
        node->isEndOfSuffix = false;
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
    crawl->isEndOfSuffix = true;
}

PSLTError plst_generate(PSLTSuffixes suffixes, PSLTNode** root) {
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

PSLTSuffixSearchResult pslt_search(PSLTNode* root, PSLTDomainFull domain) {
    struct PSLTNode *crawl;
    size_t level;
    size_t length;
    PSLTDomainFull inverted;
    
    PSLTSuffixSearchResult result;
    result.tld = NULL;
    result.icann = NULL;
    result.private = NULL;

    crawl = root;
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

void pslt_free(PSLTNode* crawl) {
    for (size_t i = 0; i < PSLT_DOMAIN_ALPHABET_SIZE; i++) {
        if (crawl->children[i] != NULL) {
            pslt_free(crawl->children[i]);
        }
    }
    free(crawl);
}