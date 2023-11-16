#include "io.h"

#include "cache.h"

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <openssl/sha.h>

int io_direxists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}

int io_fileexists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISREG(st.st_mode);
}

int io_makedir(char* dir, int append_time) {
    char dir2[strlen(dir) + 50];

    if (append_time) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(dir2, "%s_%d%02d%02d_%02d%02d%02d", dir, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        strcpy(dir2, dir);
    }

    if (!io_direxists(dir2)) {
        if(mkdir(dir2, 0700) == -1) return -1;
    }

    return 0;
}

FILE* io_openfile(IOReadWrite read, char fname[500]) {
    FILE* file;
    file = fopen(fname, read ? "rb+" : "wb+");
    return file;
}

void io_dumphex(const void* data, size_t size) {
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

void io_fwrite32(uint32_t* n, FILE* file) {
    uint32_t be;
    memcpy(&be, n, 4);
    be = htobe32(be);
    fwrite(&be, sizeof(uint32_t), 1, file);
}

void io_fwriteN(void* v, size_t s, FILE* file) {
    const int block64_2write = s / 8 + (s % 8 > 0);
    
    uint64_t be[block64_2write];
    memset(be, 0, block64_2write * 8);
    memcpy(be, v, s);

    for (int i = 0; i < block64_2write; ++i) {
        be[i] = htobe64(be[i]);
    }
    fwrite(be, 8, block64_2write, file);
}

void io_freadN(void* v, size_t s, FILE* file) {
    const int block64_2read = s / 8 + (s % 8 > 0);

    uint64_t be[block64_2read];
    memset(be, 0, block64_2read * 8);

    fread(be, 8, block64_2read, file);
    for (int i = 0; i < block64_2read; ++i) {
        be[i] = be64toh(be[i]);
    }

    memcpy(v, be, s);
}

void io_fwrite64(void* n, FILE* file) {
    uint64_t be;
    memcpy(&be, n, 8);
    be = htobe64(be);
    fwrite(&be, sizeof(uint64_t), 1, file);
}

void io_fread32(void *v, FILE* file) {
    uint32_t be;
    fread(&be, sizeof(uint32_t), 1, file);
    be = be32toh(be);
    memcpy(v, &be, 4);
}

void io_fread64(void *v, FILE* file) {
    uint64_t be;
    fread(&be, sizeof(uint64_t), 1, file);
    be = be64toh(be);
    memcpy(v, &be, 8);
}

void io_hash(void const * const object, const size_t object_size, char digest[IO_DIGEST_LENGTH]) {
    uint8_t out[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha;

    memset(out, 0, SHA256_DIGEST_LENGTH);

    SHA256_Init(&sha);
    SHA256_Update(&sha, object, sizeof(object_size));
    SHA256_Final(out, &sha);

    char hex[16] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        digest[i * 2] = hex[out[i] >> 4];
        digest[i * 2 + 1] = hex[out[i] & 0x0F];
    }
}

void io_subdigest(void const * const object, const size_t object_size, char subdigest[IO_SUBDIGEST_LENGTH]) {
    char digest[IO_DIGEST_LENGTH];
    io_hash(object, object_size, digest);
    strncpy(subdigest, digest, IO_SUBDIGEST_LENGTH);
    subdigest[IO_SUBDIGEST_LENGTH - 1] = '\0';
}

int _io_run(IOReadWrite rw, char fname[700], IOObjectFunction fn, void* obj) {
    const int fileexists = io_fileexists(fname);
    if(fileexists && rw == IO_WRITE) {
        fprintf(stderr, "Warning: file %s already exists.\n", fname);
    } else
    if (!fileexists && rw == IO_READ) {
        // fprintf(stderr, "Error: file %s not exists.\n", fname);
        return -1;
    }

    FILE* file = io_openfile(rw, fname);

    if (file == NULL) {
        fprintf(stderr, "Error: impossible to open file %s for writing:\n%s\n", fname, strerror(errno));
        return -1;
    }

    fn(rw, file, obj);

    if (fclose(file)) {
        fprintf(stderr, "Error %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int io_save(TCPC(void) obj, IOObjectID fnname, IOObjectFunction fn) {
    char fname[700] = ""; {
        char objid[IO_OBJECTID_LENGTH];
        fnname(obj, objid);
        sprintf(fname, "%s/%s.bin", CACHE_PATH, objid);
    }
    return _io_run(IO_WRITE, fname, fn, (void*) obj);
}

int io_load(char objid[IO_OBJECTID_LENGTH], IOObjectFunction fn, void* obj) {
    char fname[700] = ""; {
        sprintf(fname, "%s/%s.bin", CACHE_PATH, objid);
    }
    return _io_run(IO_READ, fname, fn, obj);
}
