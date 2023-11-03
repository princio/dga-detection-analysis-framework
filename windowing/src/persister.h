#ifndef __PERSISTER_H__
#define __PERSISTER_H__

#include "experiment.h"

typedef enum IOReadWrite {
    IO_WRITE,
    IO_READ
} IOReadWrite;

void DumpHex(const void* data, size_t size);

int persister_psets(IOReadWrite);

Source* persister_source(IOReadWrite, char[100]);

int persister_windowing(IOReadWrite rw, const Galaxy* galaxy, const Source* source, const PSet* pset, MANY(Window)* windows);

void persister_description(Sources);

void persister_test();

#endif