#include "persister.h"

#include "stratosphere.h"

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

enum Persistence_type {
    PT_Parameters,
    PT_Sources,
    PT_Windows,
    PT_WSizes
};


int _dir_exists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}


int _check_dir(Experiment* exp) {
    if (strlen(exp->name) == 0) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(exp->name, "test_%d%02d%02d_%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    char dir[strlen(exp->rootpath) + strlen(exp->name) + 10];

    sprintf(dir, "%s/%s", exp->rootpath, exp->name);

    if (strlen(dir) > 0) {
        if (_dir_exists(dir)) {
            return 0;
        }
        if (mkdir(dir, 0700) == 0) {
            return 0;
        }
    }
    return -1;
}

int _open_file(FILE** file, Experiment* exp, PersisterReadWrite read, enum Persistence_type pt, char* subname) {
    if (_check_dir(exp)) {
        perror("Impossible to create the directory");
        return -1;
    }

    char fname[50];
    switch (pt) {
        case PT_Parameters:
            sprintf(fname, "parameters");
            break;
        case PT_Sources:
            sprintf(fname, "source_%s", subname);
            break;
        case PT_Windows:
            sprintf(fname, "windows_%s", subname);
            break;
        case PT_WSizes:
            sprintf(fname, "wsizes");
            break;
    }

    char path[strlen(exp->rootpath) + strlen(exp->name) + 50];
    sprintf(path, "%s/%s/%s.bin", exp->rootpath, exp->name, fname);

    *file = fopen(path, read ? "rb+" : "wb+");
    if (!*file) {
        // printf("%s Error %s\n", read ? "Reading" : "Writing", path);
        *file = NULL;
        return -1;
    }
    // printf("%s %s\n", read ? "Reading" : "Writing", path);
    return 0;
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

void persister_test() {
    {
        FILE *file = fopen("/tmp/test.bin", "wb+");

        InfiniteValues inf = { .ninf = -2.71, .pinf = 9.908 };

        FW(inf);

        fclose(file);
    }
    {
        FILE *file = fopen("/tmp/test.bin", "rb");

        InfiniteValues inf = { .ninf = 0, .pinf = 0 };

        FR(inf);

        printf("%f\n", inf.ninf);
        printf("%f\n", inf.pinf);

        fclose(file);
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



int persister_write__windowing(Experiment* exp) {
    FILE *file;

    _open_file(&file, exp, 0, PT_WSizes, 0);
    if (file == NULL) {
        return -1;
    }
    
    int len_name = strlen(exp->name);
    FW(len_name);
    fwrite((void*) &exp->name, len_name, 1, file);

    int len_rootpath = strlen(exp->rootpath);
    FW(len_rootpath);
    fwrite((void*) &exp->rootpath, len_rootpath, 1, file);

    FW(exp->wsizes.number);

    for (int i = 0; i < exp->wsizes.number; ++i) {
        FW(exp->wsizes._[i]);
    }

    return fclose(file);
}

int persister_read__windowing(Experiment* exp) {
    FILE *file;

    _open_file(&file, exp, 1, PT_WSizes, 0);
    if (file == NULL) {
        return -1;
    }
    
    int len_name;
    FR(len_name);
    fread((void*) &exp->name, len_name, 1, file);

    int len_rootpath;
    FR(len_rootpath);
    fread((void*) &exp->rootpath, len_rootpath, 1, file);

    FR(exp->wsizes.number);

    for (int i = 0; i < exp->wsizes.number; ++i) {
        FR(exp->wsizes._[i]);
    }

    return fclose(file);
}



int persister_write__psets(Experiment* exp) {
    FILE *file;
    int32_t number;
    PSet* psets;

    _open_file(&file, exp, 0, PT_Parameters, 0);
    if (file == NULL) {
        return -1;
    }

    number = exp->psets.number;
    psets = exp->psets._;

    FW(number);

    for (int i = 0; i < number; ++i) {
        FW(psets[i].infinite_values);
        FW(psets[i].nn);
        FW(psets[i].whitelisting);
        FW(psets[i].windowing);
        FW(psets[i].id);
    }

    return fclose(file);
}

int persister_read__psets(Experiment* exp) {
    FILE *file;

    _open_file(&file, exp, 1, PT_Parameters, 0);
    if (file == NULL) {
        return -1;
    }

    FR(exp->psets.number);

    exp->psets._ = calloc(sizeof(PSet), exp->psets.number);
    
    for (int i = 0; i < exp->psets.number; ++i) {
        FR(exp->psets._[i].infinite_values);
        FR(exp->psets._[i].nn);
        FR(exp->psets._[i].whitelisting);
        FR(exp->psets._[i].windowing);
        FR(exp->psets._[i].id);
    }

    return fclose(file);
}


int persister__sources(int read, Experiment* exp, char subname[50], Sources* sources) {

    FILE *file;

    _open_file(&file, exp, read, PT_Sources, subname);
    if (file == NULL) {
        return -1;
    }

    FRWNPtr fn = read ? freadN : fwriteN;

    FRW(fn, sources->number);

    if (read) {
        sources->_ = calloc(sources->number, sizeof(Source));
    }

    for (int32_t i = 0; i < sources->number; i++) {
        FRW(fn, sources->_[i].id);
        FRW(fn, sources->_[i].galaxy_id);
        FRW(fn, sources->_[i].class);
        FRW(fn, sources->_[i].nmessages);
        FRW(fn, sources->_[i].fnreq_max);
        FRW(fn, sources->_[i].q);
        FRW(fn, sources->_[i].qr);
        FRW(fn, sources->_[i].r);
        FRW(fn, sources->_[i].capture_type);
    }

    return fclose(file);
}

int persister__windows(PersisterReadWrite read, Experiment* exp, char subname[20], Windows* windows) {
    FILE *file;

    _open_file(&file, exp, read, PT_Windows, subname);
    if (file == NULL) {
        return -1;
    }

    FRWNPtr fn = read ? freadN : fwriteN;

    FRW(fn, windows->number);

    if (read) {
        windows->_ = calloc(windows->number, sizeof(Window));
    }
    
    for (int i = 0; i < windows->number; ++i) {
        FRW(fn, windows->_[i].source_id);
        FRW(fn, windows->_[i].dataset_id);
        FRW(fn, windows->_[i].window_id);

        FRW(fn, windows->_[i].class);
        FRW(fn, windows->_[i].wcount);
        FRW(fn, windows->_[i].logit);
        FRW(fn, windows->_[i].whitelistened);
        FRW(fn, windows->_[i].dn_bad_05);
        FRW(fn, windows->_[i].dn_bad_09);
        FRW(fn, windows->_[i].dn_bad_099);
        FRW(fn, windows->_[i].dn_bad_0999);
    }

    return fclose(file);
}


void persister_description(Experiment* exp, Sources sources) {
    if (_check_dir(exp)) {
        perror("Impossible to create the directory");
        return;
    }

    char path[strlen(exp->rootpath) + strlen(exp->name) + strlen("README.md") + 50];
    sprintf(path, "%s/%s/%s", exp->rootpath, exp->name, "README.md");
    printf("%s\n", path);
    FILE* fp = fopen(path, "w");

    if (fp == NULL) {
        printf("Error file description.\n");
        return;
    }

    fprintf(fp, "# %s\n\n", exp->name);
    
    fprintf(fp, "Rootpath: %s\n\n", exp->rootpath);

    {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(fp, "Test end at: %4d/%02d/%02d, %02d:%02d:%02d.\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }



    fprintf(fp, "\n\n## WSizes\n");
    fprintf(fp, "\n\nNumber:\t%d\n\n", exp->wsizes.number);
    for (int ws = 0; ws < exp->wsizes.number - 1; ws++) {
        fprintf(fp, "%d,", exp->wsizes._[ws].value);
    }
    fprintf(fp, "%d\n\n\n", exp->wsizes._[exp->wsizes.number - 1].value);


    fprintf(fp, "## Captures\n\n");

    fprintf(fp, "Captures number: %d\n\n", sources.number);

    fprintf(fp, "id,name,capture_type,class,fnreq_max,nmessages,q,r,qr,source\n");
    for (int i = 0; i < sources.number; i++) {
        Source* c = &sources._[i];
        fprintf(fp,
            "%d,\"%s\",%d,%d,%s,%ld,%ld,%ld,%ld,%ld\n",
            c->id,
            c->name,
            c->capture_type,
            c->class,
            c->source,
            c->fnreq_max,
            c->nmessages,
            c->q,
            c->r,
            c->qr
        );
    }


    fprintf(fp, "\n\n\n## Parameters\n\n");

    fprintf(fp, "Parameters number: %d\n\n", exp->psets.number);

    fprintf(fp, "id,whitelisting,windowing,infinite_values,nn\n");
    for (int i = 0; i < exp->psets.number; i++) {
        PSet* p = &exp->psets._[i];
        fprintf(fp,
            "%d,\"(%d,%f)\",\"(%f,%f)\",%s,%s\n",
            p->id,
            p->whitelisting.rank, p->whitelisting.value,
            p->infinite_values.ninf, p->infinite_values.pinf,
            WINDOWING_NAMES[p->windowing],
            NN_NAMES[p->nn]
        );
    }

    fclose(fp);
}