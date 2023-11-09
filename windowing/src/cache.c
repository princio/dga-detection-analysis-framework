#include "cache.h"

#include "io.h"

char CACHE_PATH[500];

int cache_setpath(char path[500]) {
    if (!io_makedir(path, 0)) {
        sprintf(CACHE_PATH, "%s", path);
    }
    return 0;
}