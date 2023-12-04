
#ifndef __LISTREF_H__
#define __LISTREF_H__

#include "common.h"

typedef struct ListRefItem {
    void* item;
    struct ListRefItem* next; 
} ListRefItem;

typedef struct ListRef {
    int32_t size;
    struct ListRefItem* root;
} ListRef;

typedef struct Many {
    int32_t number;
    void** _;
} Many;

ListRef listref_create(const int32_t element_size);
void listref_init(ListRef* list, const int32_t element_size);

ListRefItem* listref_get(const ListRef* list, const int32_t index);

ListRefItem* listref_insert_copy(ListRef* lists, TCPC(void));

Many listref_to_array(ListRef list);

void listref_free(ListRef*, int freeitem);
void array_free(Many);

#endif