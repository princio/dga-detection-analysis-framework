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

char g2_iodir[PATH_MAX];

static inline G2Config _g2_config_get(G2Id id) {
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

    gat = &gatherer_of_gatherers[id];

    if (gat->size == gat->array.number) {
        g2_array(id);
    }

    if (index > gat->size) {
        LOG_ERROR("[%s] index bigger than gatherer size: %ld > %ld", g2_id_names[id], index, gat->size);
        return NULL;
    }

    return gat->array._[index];
}

void* g2_get_crawling(G2Id id, size_t index) {
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
        _g2_init();
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

void** g2_insert_item(G2Id id) {
    return &g2_insert(id)->item;
}

void* g2_alloc(G2Id id, void** item) {
    *item = calloc(1, gatherer_of_gatherers[id].element_size);
    return (*item);
}

void* g2_insert_alloc_item(G2Id id) {
    return g2_alloc(id, g2_insert_item(id));
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

void g2_io_node(G2Node* node, IOReadWrite rw, char g2_node_dir[PATH_MAX]) {
    FILE* file;
    G2Index index;
    char filepath[PATH_MAX];

    sprintf(filepath, "%s_%05ld", g2_id_names[node->g2->id], node->index);
    io_path_concat(g2_node_dir, filepath, filepath);

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

    io_path_concat(iodir, "g2/", g2_iodir);
    if (io_makedirs(g2_iodir)) {
        exit(1);
    }

    char dirpath[PATH_MAX];
    size_t size;

    if (gat->iosaved) {
        return 0;
    }

    if (IO_IS_WRITE(rw)) {
        size = gat->size;
    }

    char g2_id_iodir[PATH_MAX];

    { // g2_io_makedir
        FILE* file;
        char filepath[PATH_MAX];
        sprintf(g2_id_iodir, "%s/", g2_id_names[gat->id]);
        io_path_concat(g2_iodir, g2_id_iodir, dirpath);
        if (!io_makedirs(dirpath))
            return -1;

        io_path_concat(g2_id_iodir, "general", filepath);
        file = io_openfile(rw, filepath);
    
        if (!file)
            exit(1);
        
        FRW(size);

        if (io_closefile(file, filepath))
            exit(1);
    }

    { // crawl g2 and io each node
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

            g2_io_node(node, rw, g2_id_iodir);
        }
    }

    gat->iosaved = 1;

    return 0;
}

void g2_io_all(IOReadWrite rw) {
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

void g2_io_index(FILE* file, IOReadWrite rw, G2Id id, void** item) {
    size_t index;

    if (IO_IS_WRITE(rw)) {
        memcpy(&index, *item, sizeof(G2Index));
        FW(index);
    } else {
        FR(index);
        *item = g2_get(id, index);
    }
}