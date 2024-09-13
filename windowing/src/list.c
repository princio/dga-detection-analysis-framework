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

ListItem* list_insert_copy(List* list, TCPC(void) item) {
    ListItem** cursor = &list->root;

    while (*cursor) {
        cursor = &(*cursor)->next;
    }

    ListItem* newlistitem = calloc(1, sizeof(ListItem));
    newlistitem->item = calloc(1, list->element_size);
    memcpy(newlistitem->item, item, list->element_size);

    list->size++;

    (*cursor) = newlistitem;

    return newlistitem;
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

void list_free(List* list) {
    ListItem* cursor = list->root;

    while (cursor) {
        ListItem* cursor2free = cursor;
        cursor = cursor->next;
        free(cursor2free->item);
        free(cursor2free);
    }

    list->root = NULL;
}

void array_free(Many array) {
    free(array._);
}