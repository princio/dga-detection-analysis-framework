
#ifndef __LIST_H__
#define __LIST_H__

#include "common.h"

typedef struct ListItem {
    void* item;
    struct ListItem* next; 
} ListItem;

typedef struct List {
    int32_t size;
    struct ListItem* root;
    
    const int32_t element_size;
} List;

typedef struct Many {
    int32_t number;
    void** _;
} Many;

ListItem* list_get(const List* list, const int32_t index);

void list_insert(const List* lists, void* item);

Many list_to_array(List list);

void list_free(List, int freeitem);
void array_free(Many);

#endif