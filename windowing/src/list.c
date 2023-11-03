#include "list.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


ListItem* list_get(const List* list, const int32_t index) {
    ListItem* cursor = list->root;

    int32_t i = 0;
    while (i++ < index && cursor) {
        cursor = &cursor->next;
    }
    return cursor;
}

void list_insert(List* list, void* source) {
    ListItem** cursor = &list->root;

    while (*cursor) {
        cursor = &(*cursor)->next;
    }

    (*cursor) = calloc(1, sizeof(list->element_size));

    (*cursor)->item = source;
    (*cursor)->next = NULL;
    list->size++;
}

Many list_to_array(List list) {
    Many array;
    array.number = list.size;
    array._ = calloc(array.number, sizeof(list.element_size));

    ListItem* cursor = list.root;
    int32_t i = 0;
    while (cursor) {
        memcpy(&array._[i++], cursor->item, sizeof(list.element_size));
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