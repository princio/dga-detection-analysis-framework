#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "trie.h"

#include "analyzer/flashstart.h"
#include "analyzer/training.h"
#include "generator/1707.h"
#include "generator/training.h"

#define BUFFER_SIZE 1024
#define MAX_DOMAIN_SIZE 500


struct Domain {
    char domain[MAX_DOMAIN_SIZE];
    int is_malware;
    int is_dga;
    struct SuffixSearchResult suffixes;
};


int main() {
    setvbuf(stdout, NULL, _IONBF, 0); 

    training_run();

    
    // _1707_run();

    return 0;
}