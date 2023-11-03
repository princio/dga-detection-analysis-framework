#ifndef __WWIO_H__
#define __WWIO_H__

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>

#define FW32(A) fwrite32((void*) &A, file)
#define FW64(A) fwrite64((void*) &A, file)
#define FR32(A) fread32((void*) &A, file)
#define FR64(A) fread64((void*) &A, file)

#define FWR(R, A) { if (R) freadN((void*) &A, sizeof(A), file); else fwriteN((void*) &A, sizeof(A), file); }
#define FRW(FN, A) (*FN)((void*) &A, sizeof(A), file);
#define FW(A) fwriteN((void*) &A, sizeof(A), file)
#define FR(A) freadN((void*) &A, sizeof(A), file)

typedef enum IOReadWrite {
    PERSITER_WRITE,
    PERSITER_READ
} IOReadWrite;

typedef void (*FRWNPtr)(void* v, size_t s, FILE* file);

int dir_exists(char* dir);
int make_dir(char* dir, int append_time);

FILE* open_file(IOReadWrite read, char fname[500]);
void DumpHex(const void* data, size_t size);
void fwrite32(uint32_t* n, FILE* file);
void fwriteN(void* v, size_t s, FILE* file);
void freadN(void* v, size_t s, FILE* file);
void fwrite64(void* n, FILE* file);
void fread32(void *v, FILE* file);
void fread64(void *v, FILE* file);

#endif