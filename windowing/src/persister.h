#ifndef __PERSISTER_H__
#define __PERSISTER_H__

#include "dn.h"

typedef enum PersisterReadWrite {
    PERSITER_WRITE,
    PERSITER_READ
} PersisterReadWrite;

void DumpHex(const void* data, size_t size);

int persister_write__windowing(Experiment*);
int persister_read__windowing(Experiment*);


int persister_write__psets(Experiment*);
int persister_read__psets(Experiment*);

int persister_sources(int read, Experiment* exp, char subname[50], Sources* sources);

int persister_windows(PersisterReadWrite read, Experiment*, char subname[20], Windows*);

void persister_description(Experiment*, Sources);

void persister_test();

#endif