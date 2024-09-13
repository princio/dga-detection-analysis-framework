#ifndef __IO_H__
#define __IO_H__

#include "common.h"

#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>

#define IO_DIGEST_LENGTH 65
#define IO_SUBDIGEST_LENGTH 12
#define IO_OBJECTID_LENGTH 128

extern char windowing_iodir[PATH_MAX];

#define FW32(A) fwrite32((void*) &A, file)
#define FW64(A) fwrite64((void*) &A, file)
#define FR32(A) fread32((void*) &A, file)
#define FR64(A) fread64((void*) &A, file)

#define FWR(R, A) { if (R) freadN((void*) &A, sizeof(A), file); else fwriteN((void*) &A, sizeof(A), file); }


int __io__debug;
#ifdef IO_DEBUG
FILE* __io__log;

#define FRW(A) \
if(__io__debug) fprintf(__io__log, "%d\t%ld\t%30s\t", __LINE__, sizeof(A), #A); \
(*__FRW)((void*) &A, sizeof(A), file);

#define IOPRINT(...) \
if(__io__debug) {\
    fprintf(__io__log, "%d\t", __LINE__); \
    fprintf(__io__log, __VA_ARGS__); \
}\

#define IOLOGPATH(rw, N) { char __io__log__path[PATH_MAX]; \
    sprintf(__io__log__path, "/home/princio/Desktop/results/%s.%s", #N, rw ? "read" : "write");\
    __io__log = fopen(__io__log__path, "w");\
}
#else
#define IOLOGPATH(rw, N)
#define IOPRINT(...)
#define FRW(A) (*__FRW)((void*) &A, sizeof(A), file);
#define FRWSIZE(A, S) (*__FRW)((void*) &A, S, file);
#endif

#define FW(A) io_fwriteN((void*) &A, sizeof(A), file)
#define FR(A) io_freadN((void*) &A, sizeof(A), file)

#define IO_FLAG_LEN 80

typedef enum IOReadWrite {
    IO_WRITE,
    IO_READ
} IOReadWrite;

#define IO_IS_READ(rw) (rw == IO_READ)
#define IO_IS_WRITE(rw) (rw == IO_WRITE)
 
typedef void (*FRWNPtr)(void* v, size_t s, FILE* file);

typedef void (*IOObjectID)(TCPC(void), char[IO_OBJECTID_LENGTH]);
typedef void (*IOObjectFunction)(IOReadWrite, FILE*, void*);

FILE* io_openfile(IOReadWrite read, char fname[PATH_MAX]);
int io_closefile(FILE* file, IOReadWrite read, char path[PATH_MAX]);

int io_direxists(char* dir);
int io_fileexists(char* dir);

int io_makedir(char dir[PATH_MAX], int append_time);
int io_makedirs(char dir[PATH_MAX]);
void io_path_concat(char path1[PATH_MAX], char path2[PATH_MAX], char res[PATH_MAX]);
int io_makedirs_notoverwrite(char dirpath[DIR_MAX]);

int io_setdir(char dir[PATH_MAX]);

void io_dumphex_file(FILE* file, const void* data, size_t size);
void io_dumphex(const void* data, size_t size);
void io_fwrite32(uint32_t* n, FILE* file);
void io_fwriteN(void* v, size_t s, FILE* file);
void io_freadN(void* v, size_t s, FILE* file);
void io_fwrite64(void* n, FILE* file);
void io_fread32(void *v, FILE* file);
void io_fread64(void *v, FILE* file);

void io_hash(void const * const, const size_t, char[IO_DIGEST_LENGTH]);
void io_hash_digest(char digest[IO_DIGEST_LENGTH], uint8_t out[SHA256_DIGEST_LENGTH]);
void io_subdigest(void const * const, const size_t, char[IO_SUBDIGEST_LENGTH]);

int io_save(TCPC(void), int cache, IOObjectID fnname, IOObjectFunction fn);
int io_load(char objid[IO_OBJECTID_LENGTH], int cache, IOObjectFunction fn, void* obj);

void io_appendtime(char *str, size_t size);

void io_flag(IOReadWrite rw, FILE* file, char flag_code[IO_FLAG_LEN], int line);

#endif