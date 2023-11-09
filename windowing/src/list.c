#include "list.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

List list_create(const int32_t element_size) {
    List list = {
        .element_size = element_size
    };
    memset(&list, 0, sizeof(List));

    return list;
}

void list_init(List* list, const int32_t element_size) {
    List list2 = {
        .root = NULL,
        .size = 0,
        .element_size = element_size
    };

    memcpy(list, &list2, sizeof(List));
}

ListItem* list_get(const List* list, const int32_t index) {
    if (index > list->size) return NULL;

    ListItem* cursor = list->root;

    int32_t i = 0;
    while (i++ < index) {
        cursor = cursor->next;
    }

    return cursor;
}

void list_insert(List* list, TCPC(void) item) {
    ListItem** cursor = &list->root;

    while (*cursor) {
        cursor = &(*cursor)->next;
    }

    (*cursor) = calloc(1, list->element_size);

    (*cursor)->item = (void*) item;
    (*cursor)->next = NULL;
    list->size++;
}

Many list_to_array(List list) {
    Many array;
    array.number = list.size;
    array._ = calloc(array.number, sizeof(void*));

    ListItem* cursor = list.root;
    int32_t i = 0;
    while (cursor) {
        array._[i++] = cursor->item;
        cursor = cursor->next;
    }
    return array;
}

void list_free(List list, int freeitem) {
    ListItem* cursor = list.root;

    while (cursor) {
        ListItem* cursor2free = cursor;
        cursor = cursor->next;
        if(freeitem) {
            free(cursor2free->item);
        }
        free(cursor2free);
    }
}

void array_free(Many array) {
    free(array._);
}