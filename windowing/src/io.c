#include "io.h"

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <openssl/evp.h>

int io_direxists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}

int io_fileexists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISREG(st.st_mode);
}

int io_makedir(char* dir, int append_time) {
    if (append_time) {
        if (strlen(dir) + 50 > PATH_MAX) {
            printf("Warning: dirlen + 50 > dirlen");
        }
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(dir + strlen(dir), "_%d%02d%02d_%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    if (!io_direxists(dir)) {
        if(mkdir(dir, 0700) == -1) return -1;
    }

    return 0;
}

void io_path_concat(char *path1, char *path2, char *res) {
    assert(strlen(path1) + strlen(path2) < PATH_MAX);
    assert(path1[strlen(path1) - 1] == '/');

    char a[strlen(path1) + 50];
    char b[strlen(path2) + 50];

    strcpy(a, path1);
    strcpy(b, path2);

    snprintf(res, PATH_MAX, "%s%s", a, b);
}

int io_makedirs(char* dir) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    int error;

    error = 0;

    snprintf(tmp, PATH_MAX, "%s", dir);
    len = strlen(tmp);
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (io_direxists(tmp) == 0) {
                printf("Creating %s:\t", tmp);
                if (mkdir(tmp, S_IRWXU)) {
                    error++;
                    printf("error\n");
                } else {
                    printf("success\n");
                }
            }
            *p = '/';
        }
    }

    return error;
}

int io_makedirs_notoverwrite(char* dirpath) {

    if (io_direxists(dirpath)) {
        printf("Directory already exist: %s\n", dirpath);
        return -1;
    }

    if (io_makedirs(dirpath)) {
        printf("Impossible to create tb2 directory: %s\n", dirpath);
        return -1;
    }

    return 0;
}

FILE* io_openfile(IOReadWrite read, char* fname) {
    FILE* file;
    file = fopen(fname, read ? "rb+" : "wb+");
    if (!file) {
        LOG_ERROR("%s", strerror(errno));
    }
    return file;
}

void io_dumphex_file(FILE* file, const void* data, size_t size) {
	size_t i, j;
    size_t row_len = 128;
	for (i = 0; i < size; ++i) {
		fprintf(file, "%02X ", ((unsigned char*)data)[i]);
	}
    fprintf(file, "\n");
}

void io_dumphex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
    size_t row_len = sizeof(ascii) - 1;
	ascii[row_len] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % row_len] = ((unsigned char*)data)[i];
		} else {
			ascii[i % row_len] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % row_len == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % row_len] = '\0';
				if ((i+1) % row_len <= 8) {
					printf(" ");
				}
				for (j = (i+1) % row_len; j < row_len; ++j) {
					printf("   ");
				}
				// printf("|  %s \n", ascii);
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


/// @brief If v is the pointer to a struct of size `s` but the
///        sum of its fields is different from `s` then 
///        valgrind will give Sysbuffer uninitialized error
void io_fwriteN(void* v, size_t s, FILE* file) {
    const int block64_2write = s / 8 + (s % 8 > 0);
    
    uint64_t be[block64_2write];
    memset(be, 0, block64_2write * 8);
    memcpy(be, v, s);

    for (int i = 0; i < block64_2write; ++i) {
        be[i] = htobe64(be[i]);
    }
    fwrite(be, 8, block64_2write, file);

#ifdef IO_DEBUG
    if (__io__debug) {
        io_dumphex_file(__io__log, (void*) be, block64_2write * 8);
        fflush(__io__log);
    }
#endif

}

void io_freadN(void* v, size_t s, FILE* file) {
    const int block64_2read = s / 8 + (s % 8 > 0);

    uint64_t be[block64_2read];
    memset(be, 0, block64_2read * 8);

    int a = fread(be, 8, block64_2read, file);

#ifdef IO_DEBUG
    if (__io__debug) {
        io_dumphex_file(__io__log, (void*) be, block64_2read * 8);
        fflush(__io__log);
    }
#endif

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
    size_t _n = fread(&be, sizeof(uint32_t), 1, file);
    assert(_n == sizeof(uint64_t));
    be = be32toh(be);
    memcpy(v, &be, 4);
}

void io_fread64(void *v, FILE* file) {
    uint64_t be;
    size_t _n = fread(&be, sizeof(uint64_t), 1, file);
    assert(_n == sizeof(uint64_t));
    be = be64toh(be);
    memcpy(v, &be, 8);
}

void io_hash(void const * const object, const size_t object_size, char digest[IO_DIGEST_LENGTH]) {
    uint8_t out[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha;

    memset(out, 0, SHA256_DIGEST_LENGTH);

    {
        EVP_MD_CTX *mdctx;
        unsigned char **digest = NULL;
        unsigned int digest_len;

        if((mdctx = EVP_MD_CTX_new()) == NULL) {
            // error
            LOG_ERROR("Error hash");
        }
        if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
            // error
            LOG_ERROR("Error hash");
        }
        if(1 != EVP_DigestUpdate(mdctx, object, object_size)) {
            // error
            LOG_ERROR("Error hash");
        }
        if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL) {
            // error
            LOG_ERROR("Error hash");
        }
        if(1 != EVP_DigestFinal_ex(mdctx, *digest, &digest_len)) {
            // error
            LOG_ERROR("Error hash");
        }
        EVP_MD_CTX_free(mdctx);
    }

    char hex[16] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        digest[i * 2] = hex[out[i] >> 4];
        digest[i * 2 + 1] = hex[out[i] & 0x0F];
    }
}

void io_appendtime(char *str, size_t size) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    snprintf(str + strlen(str), size, "%02d%02d%02d_%02d%02d%02d", (tm.tm_year + 1900), tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void io_subdigest(void const * const object, const size_t object_size, char subdigest[IO_SUBDIGEST_LENGTH]) {
    char digest[IO_DIGEST_LENGTH];
    io_hash(object, object_size, digest);
    strncpy(subdigest, digest, IO_SUBDIGEST_LENGTH);
    subdigest[IO_SUBDIGEST_LENGTH - 1] = '\0';
}

void io_flag(IOReadWrite rw, FILE* file, char *flag_code, int line) {
    char flag_code_memset0[IO_FLAG_LEN];
    memset(flag_code_memset0, 0, IO_FLAG_LEN);
    strcpy(flag_code_memset0, flag_code);
    if (rw == IO_WRITE) {
        io_fwriteN(flag_code_memset0, IO_FLAG_LEN, file);
    } else {
        char flag_shouldbe[IO_FLAG_LEN];

        io_freadN(flag_shouldbe, IO_FLAG_LEN, file);
        if (strcmp(flag_code, flag_shouldbe) != 0) {
            printf("Error[%s:%d][%5s]: flag check failed: code=<%s> read=<%s>\n", __FILE__, line, rw == IO_WRITE ? "write" : "read", flag_code, flag_shouldbe);
            exit(1);
        }
    }
}