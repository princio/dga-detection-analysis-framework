#ifndef __IO_H__
#define __IO_H__

#include "common.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>

#define IO_DIGEST_LENGTH 64
#define IO_SUBDIGEST_LENGTH 12
#define IO_OBJECTID_LENGTH 128

#define FW32(A) fwrite32((void*) &A, file)
#define FW64(A) fwrite64((void*) &A, file)
#define FR32(A) fread32((void*) &A, file)
#define FR64(A) fread64((void*) &A, file)

#define FWR(R, A) { if (R) freadN((void*) &A, sizeof(A), file); else fwriteN((void*) &A, sizeof(A), file); }
#define FRW(A) (*__FRW)((void*) &A, sizeof(A), file);
#define FW(A) fwriteN((void*) &A, sizeof(A), file)
#define FR(A) freadN((void*) &A, sizeof(A), file)

typedef enum IOReadWrite {
    IO_WRITE,
    IO_READ
} IOReadWrite;

typedef void (*FRWNPtr)(void* v, size_t s, FILE* file);

typedef void (*IOObjectID)(TCPC(void), char[IO_OBJECTID_LENGTH]);
typedef void (*IOObjectFunction)(IOReadWrite, FILE*, void*);

FILE* io_openfile(IOReadWrite read, char fname[500]);

int io_direxists(char* dir);
int io_fileexists(char* dir);

int io_makedir(char dir[PATH_MAX], int append_time);
int io_makedirs(char dir[PATH_MAX]);

void io_dumphex(const void* data, size_t size);
void io_fwrite32(uint32_t* n, FILE* file);
void io_fwriteN(void* v, size_t s, FILE* file);
void io_freadN(void* v, size_t s, FILE* file);
void io_fwrite64(void* n, FILE* file);
void io_fread32(void *v, FILE* file);
void io_fread64(void *v, FILE* file);

void io_hash(void const * const, const size_t, char[IO_DIGEST_LENGTH]);
void io_subdigest(void const * const, const size_t, char[IO_SUBDIGEST_LENGTH]);

int io_save(TCPC(void), int cache, IOObjectID fnname, IOObjectFunction fn);
int io_load(char objid[IO_OBJECTID_LENGTH], int cache, IOObjectFunction fn, void* obj);

void io_appendtime(char *str, size_t size);

#endif