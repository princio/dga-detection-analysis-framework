#include "io.h"

#include "windowing.h"

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int dir_exists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}

int make_dir(char* dir, int append_time) {
    char dir2[strlen(dir) + 50];

    if (append_time) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(dir2, "%s_%d%02d%02d_%02d%02d%02d", dir, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        strcpy(dir2, dir);
    }

    if (dir_exists(dir) || mkdir(dir, 0700) == 0) {
        return 0;
    }

    return -1;
}

FILE* open_file(IOReadWrite read, char fname[500]) {
    FILE* file;
    file = fopen(fname, read ? "rb+" : "wb+");
    return file;
}

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
    printf("\n");
}

void fwrite32(uint32_t* n, FILE* file) {
    uint32_t be;
    memcpy(&be, n, 4);
    be = htobe32(be);
    fwrite(&be, sizeof(uint32_t), 1, file);
}

void fwriteN(void* v, size_t s, FILE* file) {
    // printf("Size of = %ld\n", s);
    if (s % 8 == 0) {
        uint64_t be[s/8];
        memcpy(be, v, s);
        for (size_t i = 0; i < s/8; ++i) {
            be[i] = htobe64(be[i]);
        }
        fwrite(be, s, 1, file);
    } else
    if (s % 4 == 0) {
        uint32_t be[s/4];
        memcpy(be, v, s);
        for (size_t i = 0; i < s/4; ++i) {
            be[i] = htobe32(be[i]);
        }
        fwrite(be, s, 1, file);
    } else {
        printf("Error: implement for sizes differents from 4 and 8 multiples.\n");
    }
}

void freadN(void* v, size_t s, FILE* file) {
    if (s % 8 == 0) {
        uint64_t be[s/8];
        fread(be, s, 1, file);
        for (size_t i = 0; i < s/8; ++i) {
            be[i] = be64toh(be[i]);
        }
        memcpy(v, be, s);
    } else
    if (s % 4 == 0) {
        uint32_t be[s/4];
        fread(be, s, 1, file);
        for (size_t i = 0; i < s/4; ++i) {
            be[i] = be32toh(be[i]);
        }
        memcpy(v, be, s);
    } else {
        printf("Error: implement for sizes differents from 4 and 8 multiples.\n");
    }
}

void fwrite64(void* n, FILE* file) {
    uint64_t be;
    memcpy(&be, n, 8);
    be = htobe64(be);
    fwrite(&be, sizeof(uint64_t), 1, file);
}

void fread32(void *v, FILE* file) {
    uint32_t be;
    fread(&be, sizeof(uint32_t), 1, file);
    be = be32toh(be);
    memcpy(v, &be, 4);
}

void fread64(void *v, FILE* file) {
    uint64_t be;
    fread(&be, sizeof(uint64_t), 1, file);
    be = be64toh(be);
    memcpy(v, &be, 8);
}