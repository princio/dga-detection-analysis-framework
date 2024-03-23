
#ifndef __WINDOWING_GATHERER2H__
#define __WINDOWING_GATHERER2H__

#include "common.h"

#include "io.h"

#define G2_NAME_MAX 100

#define G2_NUM 8

typedef enum G2Id {
    G2_SOURCE,
    G2_W0MANY,
    G2_WMANY,
    G2_WING,
    G2_WMC,
    G2_WFOLD,
    G2_WSPLIT,
    G2_CONFIGSUITE
} G2Id;

typedef void (*G2FreeFn)(void *);
typedef void (*G2IOFn)(IOReadWrite, FILE*, void**);
typedef void (*G2HashFn)(void*, SHA256_CTX*);
typedef void (*G2PrintFn)(void*);

typedef size_t G2Index;

typedef struct __G2* RG2;

typedef struct G2Node {
    size_t index;
    RG2 g2;
    void* item;
    struct G2Node* next;
    struct G2Node* prev;
    int iosaved;
} G2Node;

typedef struct G2Config {
    G2Id id;
    size_t size;
    size_t element_size;
    G2IOFn iofn;
    G2FreeFn freefn;
    G2PrintFn printfn;
    G2HashFn hashfn;
} G2Config;

typedef struct __G2 {
    G2Id id;

    size_t size;
    size_t element_size;

    G2IOFn iofn;
    G2FreeFn freefn;
    G2PrintFn printfn;
    G2HashFn hashfn;

    G2Node root;

    __MANY array;

    int ioloaded;
    int iostored;
} __G2;

void g2_init();

__MANY g2_array(G2Id);

void* g2_get(G2Id, size_t index);

size_t g2_size(G2Id);

void* g2_insert_alloc_item(G2Id id);

void g2_free_all();

void g2_set_iodir(char path[PATH_MAX]);

int g2_io_call(G2Id, IOReadWrite rw);
void g2_io_index(FILE* file, IOReadWrite rw, const G2Id id, void** item);
void g2_io_all(IOReadWrite rw);

#define G2_IO_HASH_UPDATE(V) SHA256_Update(sha, &(V), sizeof(V));
#define G2_IO_HASH_UPDATE_DOUBLE(V) { int64_t tmp = (int64_t) (V); SHA256_Update(sha, &tmp, sizeof(int64_t)); };

extern G2Config g2_config_source;
extern G2Config g2_config_w0many;
extern G2Config g2_config_wmany;
extern G2Config g2_config_wing;
extern G2Config g2_config_wmc;
extern G2Config g2_config_wfold;
extern G2Config g2_config_wsplit;
extern G2Config g2_config_configsuite;

#endif