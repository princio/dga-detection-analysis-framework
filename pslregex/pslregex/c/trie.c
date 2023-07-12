#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "trie.h"

#define CHAR_TO_INDEX(c) ((unsigned int)c - (unsigned int)'!')
 
// Returns new trie node (initialized to NULLs)
struct TrieNode *rootNode(void)
{
    struct TrieNode *pNode = NULL;
 
    pNode = (struct TrieNode *)malloc(sizeof(struct TrieNode));
 
    if (pNode)
    {
        int i;
 
        pNode->isEndOfWord = false;

        for (i = 0; i < ALPHABET_SIZE; ++i) pNode->children[i] = NULL;
    }
 
    return pNode;
}
 
// Returns new trie node (initialized to NULLs)
struct TrieNode *getNode()
{
    struct TrieNode *pNode = NULL;
 
    pNode = (struct TrieNode *)malloc(sizeof(struct TrieNode));
 
    if (pNode)
    {
        unsigned int i;
 
        pNode->isEndOfWord = false;
        pNode->suffix = NULL;

        for (i = 0; i < ALPHABET_SIZE; ++i) pNode->children[i] = NULL;
    }
 
    return pNode;
}
 
// If not present, inserts key into trie
// If the key is prefix of trie node, just marks leaf node
void insert(struct TrieNode *root, Suffix* suffix)
{
    int nlabels;
    int level;
    int length = strlen(suffix->suffix);
    unsigned int index;
 
    struct TrieNode *pCrawl = root;

    char inverted[strlen(suffix->suffix) + 1];
    domain_invert(suffix->suffix, inverted);
    
    for (level = 0; level < length; level++)
    {
        index = CHAR_TO_INDEX(inverted[level]);

        if (index > ALPHABET_SIZE) continue;

        if (!pCrawl->children[index]) {
            pCrawl->children[index] = getNode();
        }
 
        pCrawl = pCrawl->children[index];
    }
 
    // mark last node as leaf
    pCrawl->suffix = suffix;
    pCrawl->isEndOfWord = true;
}
 
// Returns true if key presents in trie, else false
struct SuffixSearchResult search(struct TrieNode *root, const char *key)
{
    int level;
    int length = strlen(key);
    int index;
    struct TrieNode *pCrawl = root;
    
    struct SuffixSearchResult result;
    result.tld = NULL;
    result.icann = NULL;
    result.private = NULL;

    char inverted[strlen(key) + 1];
    domain_invert(key, inverted);
 
    int first_label = 1;
    for (level = 0; level < length; level++)
    {

        index = CHAR_TO_INDEX(inverted[level]);

        if (first_label && level > 0 && inverted[level - 1] == '.') {
            first_label = 0;
        }

        if (index > ALPHABET_SIZE || index < 0) continue;
 
        if (!pCrawl->children[index]) {
            break;
        }
 
        pCrawl = pCrawl->children[index];
        if (pCrawl->suffix && (level + 1 == strlen(inverted) || inverted[level + 1] == '.')) {
            if (first_label) {
                result.tld = pCrawl;
            }
            if (pCrawl->suffix->section == Section_PrivateDomain) {
                result.private = pCrawl;
            } else {
                result.icann = pCrawl;
            }
        }
    }

    return result;
}


struct SuffixSearchResult trie_search(struct TrieNode *root, const char domain[], int print) {
    struct SuffixSearchResult found = search(root, domain);

    if (print) {
        printf("%80s\t", domain);
        printf("%30s\t", found.tld ? found.tld->suffix->suffix : "--not found--");
        printf("%30s\t", found.icann ? found.icann->suffix->suffix : "--not found--");
        printf("%30s\n", found.private ? found.private->suffix->suffix : "--not found--");
    }
    
    return found;
}

void trie_free(struct TrieNode *root);

struct TrieNode* trie_load(const char* filepath) {
    int nsuffixes;
    Suffix *suffixes;
    
    parser("iframe.csv", &suffixes, &nsuffixes);

    char output[][32] = {"Not present in trie", "Present in trie"};
 
    struct TrieNode *root = rootNode();
 
    // Construct trie
    int i;
    for (i = 0; i < nsuffixes; i++) {

        // char *suffix = suffixes[i].suffix;
        // char inv[strlen(suffix)];

        // char labels[5][64];

        // char *cursor = strchr(suffix, '.');
        // char *cursor = strchr(suffix + 1, '.');
        // strncpy(inv, cursor, )

        if (suffixes[i].is_punycode) {
            continue;
        }

        Suffix suffix = suffixes[i];

        int nlabels = 1;

        insert(root, &suffixes[i]);
    }
    
    struct SuffixSearchResult found;

    char domain[100];

    // _search(root, "aps.la.ps.oa.blokko.co.uk2");
    // _search(root, "aps.la.ps.oa.blokko.3utilities.com");
    // _search(root, "aps.la.ps.oa.blokko.localzone.xyz");
    // _search(root, "aps.la.ps.oa.blokko.nl-ams-1.baremetal.scw.cloud");
    // _search(root, "aps.la.ps.oa.blokko.dh.bytemark.co.uk");
 
    return root;
}