#ifndef TRIE_H
#define TRIE_H

#include "common.h"

#include "parser.h"

#define ALPHABET_SIZE 484

typedef enum _SuffixSearchHow {
    SuffixSearchHow_TLD,
    SuffixSearchHow_ICANN,
    SuffixSearchHow_PRIVATE
} SuffixSearchHow;


typedef struct TrieNode 
{ 
    struct TrieNode *children[ALPHABET_SIZE];
    // isEndOfWord is true if the node 
    // represents end of a word 
    int isEndOfWord;
    Suffix* suffix;
    Suffix inverted;
} TrieNode;

struct SuffixSearchResult {
    struct TrieNode *tld;
    struct TrieNode *icann;
    struct TrieNode *private;
};

struct SuffixSearchResult trie_search(struct TrieNode *root, const char domain[], int print);

struct TrieNode* trie_load(const char* filepath);

#endif