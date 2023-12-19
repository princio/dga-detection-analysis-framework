/*
 * ============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main file of the project
 *
 *        Created:  03/24/2016 19:40:56
 *
 *         Author:  Gustavo Pantuza
 *   Organization:  Software Community
 *
 * ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>



int main (int argc, char* argv[]) {


    /* Do your magic here :) */

    struct TrieNode *root = trie_load("/home/princio/Repo/princio/malware_detection_preditc_file/pslregex2/python/data/iframe.csv");


    return EXIT_SUCCESS;
}

