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
    "windowsplit",
    "configsuite",
    "wsplitdet"
};

__G2 gatherer_of_gatherers[G2_NUM];

int initialized = 0;

char g2_iodir[PATH_MAX];

static inline G2Config _g2_config_get(const G2Id id) {
    G2Config* configs[] = {
        &g2_config_source,
        &g2_config_w0many,
        &g2_config_wmany,
        &g2_config_wing,
        &g2_config_wmc,
        &g2_config_wfold,
        &g2_config_wsplit,
        &g2_config_configsuite,
        &g2_config_wsplitdet
    };
    return *configs[id];
}

__MANY g2_array(const G2Id id) {
    assert(id < G2_NUM);

    RG2 gat;

    gat = &gatherer_of_gatherers[id];

    if (gat->size == gat->array.number) {
        return gat->array;
    }

    LOG_TRACE("[%s] updating array.", g2_id_names[id]);
    
    G2Node* crawl;
    size_t index;

    MANY_FREE(gat->array);

    MANY_INITSIZE(gat->array, gat->size, sizeof(void*));


    index = 0;
    crawl = gat->root.next;
    while (crawl) {
        gat->array._[index] = crawl->item;
        crawl = crawl->next;
        index++;
    }

    return gat->array;
}

void* g2_get(const G2Id id, size_t index) {
    assert(id < G2_NUM);

    LOG_TRACE("[%s] getting %ld.", g2_id_names[id], index);
    
    RG2 gat;

    gat = &gatherer_of_gatherers[id];

    if (gat->size != gat->array.number) {
        g2_array(id);
    }

    if(index > gat->size)
        LOG_ERROR("[%s] index bigger than gatherer size: %ld > %ld", g2_id_names[id], index, gat->size);
    assert(index < gat->size);

    return gat->array._[index];
}

size_t g2_size(G2Id id) {
    return gatherer_of_gatherers[id].size;
}

void g2_init() {
    assert(initialized == 0);

    LOG_TRACE("initializing.");

    memset(gatherer_of_gatherers, 0, G2_NUM * sizeof(__G2));

    for (G2Id i = 0; i < G2_NUM; i++) {
        G2Config config = _g2_config_get(i);

        gatherer_of_gatherers[i].size = config.size;
        gatherer_of_gatherers[i].element_size = config.element_size;

        gatherer_of_gatherers[i].iofn = config.iofn;
        gatherer_of_gatherers[i].freefn = config.freefn;
        gatherer_of_gatherers[i].printfn = config.printfn;
        gatherer_of_gatherers[i].hashfn = config.hashfn;

        gatherer_of_gatherers[i].id = config.id;
    }
    initialized = 1;
}

G2Node* _g2_insert(const G2Id id) {
    assert(id < G2_NUM);

    RG2 g2;
    G2Node* crawl;

    g2 = &gatherer_of_gatherers[id];

    LOG_TRACE("[%s] insert element %ld.", g2_id_names[id], g2->size);

    crawl = &g2->root;
    while (crawl->next) {
        crawl = crawl->next;
    }
    crawl->next = calloc(1, sizeof(G2Node));
    if (crawl->next == NULL) {
        LOG_ERROR("impossible to allocate new node for gatherer %s", g2_id_names[g2->id]);
        return NULL;
    }

    crawl = crawl->next;

    crawl->item = NULL;

    crawl->next = NULL;
    crawl->prev = crawl;

    crawl->index = g2->size;
    crawl->g2 = g2;

    g2->size++;

    return crawl;
}

G2Node* _g2_insert_alloc(const G2Id id) {
    G2Node* node = _g2_insert(id);
    node->item = calloc(1, gatherer_of_gatherers[id].element_size);
    memcpy(node->item, &node->index, sizeof(G2Index));
    return node;
}

void* g2_insert_alloc_item(const G2Id id) {
    assert(initialized == 1);

    G2Node* node = _g2_insert_alloc(id);
    return node->item;
}

void _g2_free(RG2 gat) {
    LOG_TRACE("[%s] freeing.", g2_id_names[gat->id]);

    G2Node* crawl;

    crawl = gat->root.next;
    while (crawl) {
        G2Node* crawl_next = crawl->next;

        gat->freefn(crawl->item);
        free(crawl->item);
        free(crawl);
    
        crawl = crawl_next;
    }

    MANY_FREE(gat->array);
}

void g2_free_all() {
    LOG_TRACE("freeing.");
    for (size_t i = 0; i < G2_NUM; i++) {
        _g2_free(&gatherer_of_gatherers[i]);
    }
    memset(gatherer_of_gatherers, 0, sizeof(gatherer_of_gatherers));
    initialized = 0;
}

void g2_io_index(FILE* file, IOReadWrite rw, const G2Id id, void** item) {

    size_t index;

    if (IO_IS_WRITE(rw)) {
        memcpy(&index, *item, sizeof(G2Index));
        FW(index);
    } else {
        FR(index);
        *item = g2_get(id, index);
    }

    LOG_TRACE("[%s] <io-%s> of index %ld.", g2_id_names[id], IO_IS_READ(rw) ? "loaded" : "stored", index);
}

void _g2_io_node_hash_call(G2Node* node, char digest[IO_DIGEST_LENGTH]) {
    SHA256_CTX sha;
    uint8_t hash[SHA256_DIGEST_LENGTH];

    memset(hash, 0, SHA256_DIGEST_LENGTH);

    SHA256_Init(&sha);

    node->g2->hashfn(node->item, &sha);

    SHA256_Final(hash, &sha);

    io_hash_digest(digest, hash);
}

void _g2_io_node_hash(FILE* file, IOReadWrite rw, G2Node* node) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    if (node->g2->hashfn == NULL) {
        return;
    }

    char digest_computed[IO_DIGEST_LENGTH];
    char digest_stored[IO_DIGEST_LENGTH];

    strcpy(digest_computed, "not-computed");
    strcpy(digest_stored, "not-stored");
    
    _g2_io_node_hash_call(node, digest_computed);

    if (IO_IS_WRITE(rw)) {
        FW(digest_computed);
        LOG_DEBUG("[%s] index %ld write digest: %s.", g2_id_names[node->g2->id], node->index, digest_computed);
    } else {

        FR(digest_stored);

        if (memcmp(digest_computed, digest_stored, IO_DIGEST_LENGTH)) {
            LOG_DEBUG("[%s] index %ld digests not correspond.", g2_id_names[node->g2->id], node->index, digest_stored, digest_computed);
        } else {
            LOG_DEBUG("[%s] index %ld digests correspond.", g2_id_names[node->g2->id], node->index);
        }
    }
    FILE* filetmp = fopen("/tmp/bibo.csv", "a");
    fprintf(filetmp, "%s,%s,%ld,%s,%s\n",
        IO_IS_WRITE(rw) ? "write" : "read",
        g2_id_names[node->g2->id],
        node->index,
        digest_stored,
        digest_computed
    );
    fclose(filetmp);
}

void g2_io_node(G2Node* node, IOReadWrite rw, char g2_node_dir[PATH_MAX]) {
    LOG_TRACE("[%s] node %ld.", g2_id_names[node->g2->id], node->index);

    FILE* file;
    char filepath[PATH_MAX];

    sprintf(filepath, "%s_%05ld", g2_id_names[node->g2->id], node->index);
    io_path_concat(g2_node_dir, filepath, filepath);

    file = io_openfile(rw, filepath);
    if (!file)
        exit(1);

    if (IO_IS_READ(rw)) {
        memcpy(node->item, &node->index, sizeof(G2Index));
    }

    node->g2->iofn(rw, file, &node->item);

    _g2_io_node_hash(file, rw, node);

    if (io_closefile(file, rw, filepath))
        exit(1);

    node->iosaved = 1;
}

int g2_io(RG2 gat, IOReadWrite rw) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    LOG_TRACE("[%s] %s.", g2_id_names[gat->id], IO_IS_READ(rw) ? "loading" : "storing");

    char dirpath[PATH_MAX];
    size_t size;

    if (IO_IS_WRITE(rw) && gat->iostored) {
        return 0;
    }
    if (IO_IS_READ(rw) && gat->ioloaded) {
        return 0;
    }

    { // g2_io_makedir for node
        FILE* file;
        char filepath[PATH_MAX];
        sprintf(dirpath, "%s/", g2_id_names[gat->id]);
        io_path_concat(g2_iodir, dirpath, dirpath);
        if (IO_IS_WRITE(rw)) {
            if (io_makedirs(dirpath)) {
                LOG_ERROR("[%s] impossible to create store directory.", g2_id_names[gat->id]);
                return -1;
            }
        } else {
            if (!io_direxists(dirpath)) {
                LOG_ERROR("[%s] impossible to load, directory not exist.", g2_id_names[gat->id]);
                return -1;
            }
        }

        io_path_concat(dirpath, "general", filepath);
        file = io_openfile(rw, filepath);
        if (!file)
            exit(1);
        

        if (IO_IS_WRITE(rw)) {
            size = gat->size;
        }
        FRW(size);

        if (io_closefile(file, rw, filepath))
            exit(1);
    }

    { // crawl g2 and io each node

        if (IO_IS_READ(rw)) {
            for (G2Index index = 0; index < size; index++) {
                G2Node* node = _g2_insert_alloc(gat->id);
                memcpy(node->item, &index, sizeof(G2Index));
                g2_io_node(node, rw, dirpath);
            }
        } else {
            G2Node* node;
            node = gat->root.next;
            for (G2Index index = 0; index < size; index++) {
                g2_io_node(node, rw, dirpath);
                node = node->next;
            }
        }
    }

    if (IO_IS_WRITE(rw)) {
        gat->iostored = 1;
    } else {
        gat->ioloaded = 1;
    }
    return 0;
}

int g2_io_setroot(IOReadWrite rw) {
    io_path_concat(windowing_iodir, "g2/", g2_iodir);
    if (IO_IS_WRITE(rw)) {
        if (io_makedirs(g2_iodir)) {
            LOG_ERROR("impossible to create store directory.");
            return -1;
        }
    } else {
        if (!io_direxists(g2_iodir)) {
            LOG_ERROR("impossible to load, directory not exist.");
            return -1;
        }
    }
    return 0;
}

int g2_io_call(const G2Id id, IOReadWrite rw) {
    if (g2_io_setroot(rw)) {
        return -1;
    }

    RG2 g2 = &gatherer_of_gatherers[id];

    if (IO_IS_WRITE(rw) && g2->iostored) {
        LOG_TRACE("[%s] calling for storing already done.", g2_id_names[id]);
        return 0;
    }
    if (IO_IS_READ(rw) && g2->ioloaded) {
        LOG_TRACE("[%s] calling for loading already done.", g2_id_names[id]);
        return 0;
    }
    LOG_TRACE("[%s] calling %s.", g2_id_names[id], IO_IS_READ(rw) ? "loading" : "storing");
    
    return g2_io(&gatherer_of_gatherers[id], rw);
}

int g2_io_all(IOReadWrite rw) {
    assert(initialized == 1);

    if (g2_io_setroot(rw)) {
        return -1;
    }

    LOG_TRACE("%s all.", IO_IS_READ(rw) ? "Loading" : "Storing");

    for (size_t i = 0; i < G2_NUM; i++) {
        g2_io(&gatherer_of_gatherers[i], rw);
    }

    return 0;
}