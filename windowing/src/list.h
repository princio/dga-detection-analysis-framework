
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

List list_create(const int32_t element_size);
void list_init(List* list, const int32_t element_size);

ListItem* list_get(const List* list, const int32_t index);

void list_insert(List* lists, TCPC(void));

Many list_to_array(List list);

void list_free(List, int freeitem);
void array_free(Many);

#endif