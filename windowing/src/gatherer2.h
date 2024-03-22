
#ifndef __WINDOWING_GATHERER2H__
#define __WINDOWING_GATHERER2H__

#include "common.h"

#include "io.h"

#define G2_NAME_MAX 100

#define G2_NUM 7

typedef enum G2Id {
    G2_SOURCE,
    G2_W0MANY,
    G2_WMANY,
    G2_WING,
    G2_WMC,
    G2_WFOLD,
    G2_WSPLIT
} G2Id;

typedef void (*G2FreeFn)(void *);
typedef void (*G2IOFn)(IOReadWrite, FILE*, void**);

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
} G2Config;

typedef struct __G2 {
    G2Id id;

    size_t size;
    size_t element_size;

    G2IOFn iofn;
    G2FreeFn freefn;

    G2Node* root;

    __MANY array;

    int iosaved;
} __G2;

__MANY g2_array(G2Id);

void* g2_get(G2Id, size_t index);

void* g2_insert_alloc_item(G2Id id);

void g2_free();

void g2_set_iodir(char path[PATH_MAX]);

int g2_io_call(G2Id, IOReadWrite rw);
void g2_io_index(FILE* file, IOReadWrite rw, G2Id id, void** item);

extern G2Config g2_config_source;
extern G2Config g2_config_w0many;
extern G2Config g2_config_wmany;
extern G2Config g2_config_wing;
extern G2Config g2_config_wmc;
extern G2Config g2_config_wfold;
extern G2Config g2_config_wsplit;

extern char g2_name[G2_NAME_MAX];

#endif