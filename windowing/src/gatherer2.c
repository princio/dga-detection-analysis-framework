#include "gatherer2.h"

#include "io.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char g2_id_names[G2_NUM][G2_NAME_MAX] = {
    "source",
    "window0many",
    "windowmany",
    "windowing",
    "windowmc",
    "windowfold",
    "windowsplit"
};

__G2 gatherer_of_gatherers[G2_NUM];

int initialized = 0;

inline G2Config _g2_config_get(G2Id id) {
    G2Config* configs[] = {
        &g2_config_source,
        &g2_config_w0many,
        &g2_config_wmany,
        &g2_config_wing,
        &g2_config_wmc,
        &g2_config_wfold,
        &g2_config_wsplit
    };
    return *configs[id];
}

__MANY g2_array(G2Id id) {
    assert(id < G2_NUM);

    RG2 gat;

    gat = &gatherer_of_gatherers[id];

    if (gat->size == gat->array.number) {
        return gat->array;
    }
    
    __MANY many;
    G2Node* crawl;
    size_t index;

    MANY_FREE(gat->array);

    MANY_INITSIZE(many, gat->size, sizeof(void*));

    crawl = gat->root;

    index = 0;
    while (crawl) {
        many._[index] = crawl->item;
        crawl = crawl->next;
    }

    return many;
}

void* g2_get(G2Id id, size_t index) {
    assert(id < G2_NUM);
    
    RG2 gat;
    G2Node* crawl;

    gat = &gatherer_of_gatherers[id];
    
    LOG_DEBUG("getting element %s[%ld].", g2_id_names[gat->id], index);

    if (index > gat->size) {
        LOG_ERROR("index bigger than gatherer size: %ld > %ld", index, gat->size);
        return NULL;
    }

    crawl = gat->root;
    for (size_t i = 0; i < index; i++) {
        crawl = crawl->next;
    }

    assert(crawl->index != index);
    
    return crawl->item;
}

void _g2_init() {
    assert(initialized == 0);

    memset(gatherer_of_gatherers, 0, G2_NUM * sizeof(__G2));

    for (G2Id i = 0; i < G2_NUM; i++) {
        G2Config config = _g2_config_get(i);

        gatherer_of_gatherers[i].size = config.size;
        gatherer_of_gatherers[i].element_size = config.element_size;

        gatherer_of_gatherers[i].iofn = config.iofn;
        gatherer_of_gatherers[i].freefn = config.freefn;

        gatherer_of_gatherers[i].id = config.id;
    }
    initialized = 1;
}

G2Node* g2_insert(G2Id id) {
    assert(id < G2_NUM);

    if (!initialized) {
        g2_init();
    }
    
    RG2 g2;
    G2Node* crawl;

    g2 = &gatherer_of_gatherers[id];

    crawl = g2->root;
    while (crawl) {
        crawl = crawl->next;
    }
    crawl->next = calloc(1, sizeof(G2Node));
    if (crawl->next == NULL) {
        LOG_ERROR("impossible to allocate new node for gatherer %s", g2_id_names[g2->id]);
        return NULL;
    }

    crawl->next->item = NULL;
    crawl->next->prev = crawl;

    crawl->next->index = g2->size;
    g2->size++;

    return crawl->next;
}

void* g2_insert_and_alloc(G2Id id) {
    RG2 g2;
    G2Node* crawl;
    void* newelement;
    
    crawl = g2_insert(id);

    g2 = &gatherer_of_gatherers[id];

    newelement = calloc(1, g2->element_size);
    if (newelement == NULL) {
        LOG_ERROR("impossible to allocate new element for gatherer %s", g2_id_names[g2->id]);
        free(crawl->next);
        return NULL;
    }

    // setting g2index
    crawl->item = newelement;
    memcpy(newelement, &g2->size, sizeof(size_t));

    return newelement;
}

void g2_free(RG2 gat) {
    G2Node* crawl;

    crawl = gat->root;
    while (crawl) {
        G2Node* current = crawl;
        crawl = crawl->next;
        gat->freefn(current->item);
        free(current);
    }
}

void g2_free_all() {
    for (size_t i = 0; i < G2_NUM; i++) {
        g2_free(&gatherer_of_gatherers[i]);
    }
}

void g2_io_g2_node(G2Node* node, IOReadWrite rw) {
    FILE* file;
    G2Index index;
    char filepath[PATH_MAX];

    sprintf(filepath, "%s/%s_%05ld", g2_id_names[node->g2->id], node->index);
    io_path_concat(iodir, filepath, filepath);

    file = io_openfile(rw, filepath);
    if (!file)
        exit(1);
    
    node->g2->iofn(rw, file, &node->item);

    memcpy(&index, node->item, sizeof(G2Index)); // before iofn item is not allocated

    if (io_closefile(file, filepath))
        exit(1);

    node->iosaved = 1;
}

int g2_io(RG2 gat, IOReadWrite rw) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    char dirpath[PATH_MAX];
    size_t size;

    if (gat->iosaved) {
        return 0;
    }

    if (IO_IS_WRITE(rw)) {
        size = gat->size;
    }

    {
        FILE* file;
        char filepath[PATH_MAX];
        char dirname[PATH_MAX];
        sprintf(dirname, "%s/", g2_id_names[gat->id]);
        io_path_concat(iodir, dirname, dirpath);
        if (!io_makedirs(dirpath))
            return -1;

        io_path_concat(iodir, "general", filepath);
        file = io_openfile(rw, filepath);
    
        if (!file)
            exit(1);
        
        FRW(size);

        if (io_closefile(file, filepath))
            exit(1);
    }

    G2Node* node = gat->root;
    for (size_t index = 0; index < size; index++) {
        FILE* file;
        G2Index index;
        char filepath[PATH_MAX];

        if (IO_IS_READ(rw)) {
            node = g2_insert(gat->id);
        } else {
            node = node->next;
        }

        assert(node);

        g2_io_g2_node(node, rw);
    }

    gat->iosaved = 1;

    return 0;
}

int g2_io_all(IOReadWrite rw) {
    if (initialized == 0) {
        _g2_init();
    }

    for (size_t i = 0; i < G2_NUM; i++) {
        g2_io(&gatherer_of_gatherers[i], rw);
    }
}

int g2_io_call(G2Id id, IOReadWrite rw) {
    return g2_io(&gatherer_of_gatherers[id], rw);
}