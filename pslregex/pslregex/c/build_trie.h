
#include "parse_suffixes.h"

#define ALPHABET_SIZE 484

typedef enum _SuffixSearchHow {
    SuffixSearchHow_TLD,
    SuffixSearchHow_ICANN,
    SuffixSearchHow_PRIVATE
} SuffixSearchHow;


struct TrieNode 
{ 
     struct TrieNode *children[ALPHABET_SIZE];
     // isEndOfWord is true if the node 
     // represents end of a word 
     bool isEndOfWord;
     Suffix* suffix;
     char inverted[MAX_SUFFIX_SIZE];
};

struct SuffixSearchResult {
    struct TrieNode *tld;
    struct TrieNode *icann;
    struct TrieNode *private;
};