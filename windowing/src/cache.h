#ifndef __CACHE_H__
#define __CACHE_H__

#include "io.h"

extern char ROOT_PATH[500];
extern char CACHE_PATH[500];

int cache_setroot(char path[300]);
int cache_settestbed(char objid[IO_OBJECTID_LENGTH]);

#endif