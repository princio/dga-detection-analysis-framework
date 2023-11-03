#include "persister.h"

#include "windowing.h"

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define FW32(A) fwrite32((void*) &A, file)
#define FW64(A) fwrite64((void*) &A, file)
#define FR32(A) fread32((void*) &A, file)
#define FR64(A) fread64((void*) &A, file)

#define FWR(R, A) { if (R) freadN((void*) &A, sizeof(A), file); else fwriteN((void*) &A, sizeof(A), file); }
#define FRW(FN, A) (*FN)((void*) &A, sizeof(A), file);
#define FW(A) fwriteN((void*) &A, sizeof(A), file)
#define FR(A) freadN((void*) &A, sizeof(A), file)

typedef void (*FRWNPtr)(void* v, size_t s, FILE* file);

int dir_exists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}


int check_dir() {
    if (strlen(experiment.name) == 0) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(experiment.name, "test_%d%02d%02d_%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    char dir[strlen(experiment.rootpath) + strlen(experiment.name) + 10];

    sprintf(dir, "%s/%s", experiment.rootpath, experiment.name);

    if (strlen(dir) > 0) {
        if (dir_exists(dir)) {
            return 0;
        }
        if (mkdir(dir, 0700) == 0) {
            return 0;
        }
    }
    return -1;
}

FILE* open_file(IOReadWrite read, char fname[200]) {
    FILE* file;

    if (check_dir()) {
        perror("Impossible to create the directory");
        return NULL;
    }

    char path[strlen(experiment.rootpath) + strlen(experiment.name) + 200];
    sprintf(path, "%s/%s/%s.bin", experiment.rootpath, experiment.name, fname);

    file = fopen(path, read ? "rb+" : "wb+");
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


int persister_psets(IOReadWrite rw) {
    FILE *file;
    int32_t number;
    char fname[200];

    sprintf(fname, "parameters.bin");

    file = open_file(rw, fname);
    if (file == NULL) {
        return -1;
    }

    FRWNPtr fn = rw ? freadN : fwriteN;

    FRW(fn, experiment.psets.number);

    if (rw) {
        experiment.psets._ = calloc(experiment.psets.number, sizeof(Source));
    }

    for (int i = 0; i < experiment.psets.number; ++i) {
        FRW(fn, experiment.psets._[i].infinite_values);
        FRW(fn, experiment.psets._[i].nn);
        FRW(fn, experiment.psets._[i].whitelisting);
        FRW(fn, experiment.psets._[i].windowing);
        FRW(fn, experiment.psets._[i].wsize);
    }

    return fclose(file);
}

Source* persister_source(IOReadWrite rw, char name[100]) {

    FILE *file;
    Source* source;
    char fname[200];

    source = NULL;

    snprintf(fname, "source_%s_%d.bin", 200, name);

    file = open_file(rw, fname);
    if (file == NULL) {
        return NULL;
    }

    source = calloc(1, sizeof(Source));

    FRWNPtr fn = rw ? freadN : fwriteN;

    FRW(fn, source->dgaclass);
    FRW(fn, source->fnreq_max);
    FRW(fn, source->q);
    FRW(fn, source->qr);
    FRW(fn, source->r);
    FRW(fn, source->capture_type);

    fclose(file);

    return source;
}

int persister_windowing(IOReadWrite rw, const WindowingGalaxy* wg, const WindowingSource* ws, const PSet* pset, MANY(Window)* windows) {
    FILE *file;
    char fname[200];
    char subdigest[9];

    strncpy(subdigest, pset->digest, sizeof subdigest);
    sprintf(fname, "%s_%s_%s", wg->name, ws->source.name, subdigest);

    file = open_file(rw, fname);
    if (file == NULL) {
        return -1;
    }

    FRWNPtr fn = rw ? freadN : fwriteN;

    FRW(fn, windows->number);

    if (rw) {
        windows->_ = calloc(windows->number, sizeof(Window));
    }
    
    for (int i = 0; i < windows->number; ++i) {
        Window* window = &windows->_[i];

        FRW(fn, window->source_index);
        FRW(fn, window->pset_index);
        
        FRW(fn, window->wnum);

        FRW(fn, window->dgaclass);
        FRW(fn, window->wcount);
        FRW(fn, window->logit);
        FRW(fn, window->whitelistened);
        FRW(fn, window->dn_bad_05);
        FRW(fn, window->dn_bad_09);
        FRW(fn, window->dn_bad_099);
        FRW(fn, window->dn_bad_0999);
    }

    return fclose(file);
}


void persister_description(MANY(Source) sources) {
    if (check_dir()) {
        perror("Impossible to create the directory");
        return;
    }

    char path[strlen(experiment.rootpath) + strlen(experiment.name) + strlen("README.md") + 50];
    sprintf(path, "%s/%s/%s", experiment.rootpath, experiment.name, "README.md");
    printf("%s\n", path);
    FILE* fp = fopen(path, "w");

    if (fp == NULL) {
        printf("Error file description.\n");
        return;
    }

    fprintf(fp, "# %s\n\n", experiment.name);
    
    fprintf(fp, "Rootpath: %s\n\n", experiment.rootpath);

    {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(fp, "Test end at: %4d/%02d/%02d, %02d:%02d:%02d.\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    fprintf(fp, "## Captures\n\n");

    fprintf(fp, "Captures number: %d\n\n", sources.number);

    fprintf(fp, "id,galaxy_id,name,capture_type,class,fnreq_max,q,r,qr,source\n");
    for (int i = 0; i < sources.number; i++) {
        Source* c = &sources._[i];
        fprintf(fp,
            "%d,%d,\"%s\",%d,%d,%s,%ld,%ld,%ld,%ld\n",
            c->index.binary,
            c->index.galaxy,
            c->name,
            c->capture_type,
            c->dgaclass,
            0,
            c->fnreq_max,
            c->q,
            c->r,
            c->qr
        );
    }


    fprintf(fp, "\n\n\n## Parameters\n\n");

    fprintf(fp, "Parameters number: %d\n\n", experiment.psets.number);

    fprintf(fp, "id,whitelisting,windowing,infinite_values,nn\n");
    for (int i = 0; i < experiment.psets.number; i++) {
        PSet* p = &experiment.psets._[i];
        fprintf(fp,
            "\"(%d,%f)\",\"(%f,%f)\",%s,%s\n",
            p->whitelisting.rank, p->whitelisting.value,
            p->infinite_values.ninf, p->infinite_values.pinf,
            WINDOWING_NAMES[p->windowing],
            NN_NAMES[p->nn]
        );
    }

    fclose(fp);
}
