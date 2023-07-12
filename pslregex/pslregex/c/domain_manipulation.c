#include <stdio.h>

#include "build_trie.h"
#include "domain_manipulation.h"
#include "parse_suffixes.h"

void load_domains_from_csv(char *filepath, char delimitator, int column) {

    Row row;
    FILE* fp;
    int lines; // header count;

    char buffer[sizeof(row)];

    fp = fopen(filepath, "r");
 
    if (!fp){
        printf("Can't open file\n");
        exit(1);
    }

    lines = -1;
    while(fgets(buffer, sizeof(row), fp)) {
        ++lines;
    }

    Domain domains[lines];

    for (int i = 0; i < lines; i++) {
        
    }


}
