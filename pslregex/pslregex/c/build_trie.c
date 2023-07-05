#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "build_trie.h"
#include "parse_suffixes.h"

#define CHAR_TO_INDEX(c) ((unsigned int)c - (unsigned int)'!')

struct TrieNode 
{ 
     struct TrieNode *children[ALPHABET_SIZE];
     // isEndOfWord is true if the node 
     // represents end of a word 
     bool isEndOfWord;
     Suffix* suffix;
};


 
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
    int level;
    int length = strlen(suffix->suffix);
    unsigned int index;
 
    struct TrieNode *pCrawl = root;
 
    for (level = 0; level < length; level++)
    {
        index = CHAR_TO_INDEX(suffix->suffix[level]);

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
bool search(struct TrieNode *root, const char *key)
{
    int level;
    int length = strlen(key);
    int index;
    struct TrieNode *pCrawl = root;
 
    printf("\n   key: %s\n", key);
    for (level = 0; level < length; level++)
    {
        index = CHAR_TO_INDEX(key[level]);

        if (index > ALPHABET_SIZE || index < 0) continue;
 
        if (!pCrawl->children[index]) {
            printf("Not found :(\n\n");
            return false;
        }
 
        pCrawl = pCrawl->children[index];
    }
 
    if (pCrawl->suffix->suffix) {
        printf(" found: %s\n", pCrawl->suffix->suffix);
    } else {
        printf("Something is strane :/");
    }
    printf("eoword? %s", pCrawl->isEndOfWord ? "yes" : "no");
    printf("\n\n");

    return (pCrawl->isEndOfWord);
}


// Driver
int main()
{
    int nsuffixes;
    Suffix *suffixes;
    
    load_suffixes("iframe.csv", &suffixes, &nsuffixes);

    printf("%d", nsuffixes);
 
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

        insert(root, &suffixes[i]);
    }
    
 
    // Search for different keys
    // search(root, "tuyenquang.vn");
    search(root, "co.uk2");
    search(root, "uk");
    search(root, "3utilities.com");
    search(root, "localzone.xyz");
    search(root, "nl-ams-1.baremetal.scw.cloud");
 
    return 0;
}