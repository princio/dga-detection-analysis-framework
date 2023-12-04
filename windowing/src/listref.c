#include "listref.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ListRef listref_create(const int32_t element_size) {
    ListRef list;
    memset(&list, 0, sizeof(ListRef));

    return list;
}

void listref_init(ListRef* list, const int32_t element_size) {
    ListRef list2 = {
        .root = NULL,
        .size = 0,
    };

    memcpy(list, &list2, sizeof(ListRef));
}

ListRefItem* listref_get(const ListRef* list, const int32_t index) {
    if (index > list->size) return NULL;

    ListRefItem* cursor = list->root;

    int32_t i = 0;
    while (i++ < index) {
        cursor = cursor->next;
    }

    return cursor;
}

ListRefItem* listref_insert(ListRef* list, TCPC(void) item) {
    ListRefItem** cursor = &list->root;

    while (*cursor) {
        cursor = &(*cursor)->next;
    }

    (*cursor) = calloc(1, sizeof(ListRefItem));

    (*cursor)->item = (void*) item;
    (*cursor)->next = NULL;
    list->size++;

    return (*cursor);
}

Many listref_to_array(ListRef list) {
    Many array;
    array.number = list.size;
    array._ = calloc(array.number, sizeof(void*));

    ListRefItem* cursor = list.root;
    int32_t i = 0;
    while (cursor) {
        array._[i++] = cursor->item;
        cursor = cursor->next;
    }
    return array;
}

void listref_free(ListRef* list, int freeitem) {
    ListRefItem* cursor = list->root;

    while (cursor) {
        ListRefItem* cursor2free = cursor;
        cursor = cursor->next;
        if(freeitem) {
            free(cursor2free->item);
        }
        free(cursor2free);
    }
}