#include "cache.h"

#include "io.h"

#include <string.h>

char ROOT_PATH[500];
char CACHE_PATH[500];

int cache_setroot(char path[300]) {
    if (!io_makedir(path, 0)) {
        sprintf(ROOT_PATH, "%s", path);
    }

    char path2[500];
    strcpy(path2, ROOT_PATH);
    sprintf(path2 + strlen(path2), "/cache");
    if (!io_makedir(path2, 0)) {
        sprintf(CACHE_PATH, "%s", path2);
    }
    return 0;
}

int cache_settestbed(char objid[IO_OBJECTID_LENGTH]) {
    char path[500];

    strcpy(path, ROOT_PATH);

    snprintf(path + strlen(path), 500, "/%s", objid);
    if (!io_makedir(path, 0)) {
        sprintf(ROOT_PATH, "%s", path);
    }

    return 0;
}