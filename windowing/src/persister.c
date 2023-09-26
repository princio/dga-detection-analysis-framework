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

#define FW(A) fwriteN((void*) &A, sizeof(A), file)
#define FR(A) freadN((void*) &A, sizeof(A), file)

enum Persistence_type {
    PT_Parameters,
    PT_Captures,
    PT_CaptureWSets,
    PT_WSizes
};


int _dir_exists(char* dir) {
    struct stat st = {0};
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}


int _check_dir(WindowingPtr windowing) {
    if (strlen(windowing->name) == 0) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(windowing->name, "test_%d%02d%02d_%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    char dir[strlen(windowing->rootpath) + strlen(windowing->name) + 10];

    sprintf(dir, "%s/%s", windowing->rootpath, windowing->name);

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

int _open_file(FILE** file, WindowingPtr windowing, int read, enum Persistence_type pt, int id) {
    if (_check_dir(windowing)) {
        perror("Impossible to create the directory");
        return -1;
    }

    char fname[50];
    switch (pt) {
        case PT_Parameters:
            sprintf(fname, "parameters");
            break;
        case PT_Captures:
            sprintf(fname, "captures");
            break;
        case PT_CaptureWSets:
            sprintf(fname, "captures_windows_%d", id);
            break;
        case PT_WSizes:
            sprintf(fname, "wsizes");
            break;
    }

    char path[strlen(windowing->rootpath) + strlen(windowing->name) + 50];
    sprintf(path, "%s/%s/%s.bin", windowing->rootpath, windowing->name, fname);

    *file = fopen(path, read ? "rb+" : "wb+");
    if (!*file) {
        printf("%s Error %s\n", read ? "Reading" : "Writing", path);
        *file = NULL;
        return -1;
    }
    printf("%s %s\n", read ? "Reading" : "Writing", path);
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



int persister_write__windowing(WindowingPtr windowing) {
    FILE *file;

    _open_file(&file, windowing, 0, PT_WSizes, 0);
    if (file == NULL) {
        return -1;
    }
    
    int len_name = strlen(windowing->name);
    FW(len_name);
    fwrite((void*) &windowing->name, len_name, 1, file);

    int len_rootpath = strlen(windowing->rootpath);
    FW(len_rootpath);
    fwrite((void*) &windowing->rootpath, len_rootpath, 1, file);

    FW(windowing->wsizes.number);

    for (int i = 0; i < windowing->wsizes.number; ++i) {
        FW(windowing->wsizes._[i]);
    }

    return fclose(file);
}

int persister_read__windowing(WindowingPtr windowing) {
    FILE *file;

    _open_file(&file, windowing, 1, PT_WSizes, 0);
    if (file == NULL) {
        return -1;
    }
    
    int len_name;
    FR(len_name);
    fread((void*) &windowing->name, len_name, 1, file);

    int len_rootpath;
    FR(len_rootpath);
    fread((void*) &windowing->rootpath, len_rootpath, 1, file);

    FR(windowing->wsizes.number);

    for (int i = 0; i < windowing->wsizes.number; ++i) {
        FR(windowing->wsizes._[i]);
    }

    return fclose(file);
}



int persister_write__psets(WindowingPtr windowing) {
    FILE *file;
    int32_t number;
    PSet* psets;

    _open_file(&file, windowing, 0, PT_Parameters, 0);
    if (file == NULL) {
        return -1;
    }

    number = windowing->psets.number;
    psets = windowing->psets._;

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

int persister_read__psets(WindowingPtr windowing) {
    FILE *file;

    _open_file(&file, windowing, 1, PT_Parameters, 0);
    if (file == NULL) {
        return -1;
    }

    FR(windowing->psets.number);

    windowing->psets._ = calloc(sizeof(PSet), windowing->psets.number);
    
    for (int i = 0; i < windowing->psets.number; ++i) {
        FR(windowing->psets._[i].infinite_values);
        FR(windowing->psets._[i].nn);
        FR(windowing->psets._[i].whitelisting);
        FR(windowing->psets._[i].windowing);
        FR(windowing->psets._[i].id);
    }

    return fclose(file);
}


int persister_write__captures(WindowingPtr windowing) {
    FILE *file;
    int32_t number;
    Captures* captures;

    _open_file(&file, windowing, 0, PT_Captures, 0);
    if (file == NULL) {
        return -1;
    }

    number = windowing->captures.number;
    captures = &windowing->captures;

    FW(number);

    for (int i = 0; i < number; ++i) {
        FW(captures->_[i].id);
        FW(captures->_[i].class);
        FW(captures->_[i].nmessages);
        FW(captures->_[i].fnreq_max);
        FW(captures->_[i].q);
        FW(captures->_[i].qr);
        FW(captures->_[i].r);
        FW(captures->_[i].capture_type);
    }

    return fclose(file);
}

int persister_read__captures(WindowingPtr windowing) {
    FILE *file;
    int32_t number;
    Captures* captures;

    _open_file(&file, windowing, 1, PT_Captures, 0);
    if (file == NULL) {
        return -1;
    }

    FR(number);

    windowing->captures.number = number;

    captures = &windowing->captures;
    for (int i = 0; i < number; ++i) {
        FR(captures->_[i].id);
        FR(captures->_[i].class);
        FR(captures->_[i].nmessages);
        FR(captures->_[i].fnreq_max);
        FR(captures->_[i].q);
        FR(captures->_[i].qr);
        FR(captures->_[i].r);
        FR(captures->_[i].capture_type);

        if (captures->_[i].capture_type == CAPTURETYPE_PCAP) {
            captures->_[i].fetch = (FetchPtr) &stratosphere_procedure;
        }
    }

    return fclose(file);
}


int persister_write__capturewsets(WindowingPtr windowing, int32_t capture_index) {

    FILE *file;
    WSet* capture_wsets;

    _open_file(&file, windowing, 0, PT_CaptureWSets, capture_index);
    if (file == NULL) {
        return -1;
    }

    capture_wsets = windowing->captures_wsets[capture_index];

    for (int w = 0; w < windowing->wsizes.number; ++w) {
        WSetPtr capture_windowing = &capture_wsets[w];
        const int32_t n_windows = capture_windowing->number;

        FW(n_windows);

        for (int r = 0; r < n_windows; ++r) {
            Window* window = &capture_windowing->_[r];

            FW(window->wnum);

            int S = window->metrics.number * sizeof(WindowMetricSet);

            fwriteN(window->metrics._, S, file);


            // {
            //     FILE* ff = fopen("./ciao.csv", "a");
            //     fprintf(ff, "%s,%d,%d,%d,%d,", try_name, trys, capture_index, w, r);
            //     uint8_t* a = (uint8_t*) window->metrics._;
            //     for (int q = 0; q < S; ++q) {
            //         fprintf(ff, "%02X", a[q]);
            //     }
            //     fprintf(ff, "\n");
            //     fclose(ff);
            // }
        }
    }

    return fclose(file);
}

int persister_read__capturewsets(WindowingPtr windowing, int32_t capture_index) {

    FILE *file;
    WSet* capture_wsets;

    _open_file(&file, windowing, 1, PT_CaptureWSets, capture_index);
    if (file == NULL) {
        return -1;
    }


    capture_wsets = windowing->captures_wsets[capture_index];
    for (int w = 0; w < windowing->wsizes.number; ++w) {
        WSet* capture_windowing = &capture_wsets[w];

        FR(capture_windowing->number);

        const int32_t n_windows = capture_windowing->number;
        for (int r = 0; r < n_windows; ++r) {
            Window* window = &capture_windowing->_[r];

            FR(window->wnum);
            
            int S = window->metrics.number * sizeof(WindowMetricSet);

            freadN(window->metrics._, S, file);

            // {
            //     FILE* ff = fopen("./ciao.csv", "a");
            //     fprintf(ff, "%s,%d,%d,%d,%d,", try_name, trys, capture_index, w, r);
            //     uint8_t* a = (uint8_t*) window->metrics._;
            //     for (int q = 0; q < S; ++q) {
            //         fprintf(ff, "%02X", a[q]);
            //     }
            //     fprintf(ff, "\n");
            //     fclose(ff);
            // }
        }
    }
    
    return fclose(file);
}


void persister_description(WindowingPtr windowing) {
    if (_check_dir(windowing)) {
        perror("Impossible to create the directory");
        return;
    }

    char path[strlen(windowing->rootpath) + strlen(windowing->name) + strlen("README.md") + 50];
    sprintf(path, "%s/%s/%s", windowing->rootpath, windowing->name, "README.md");
    printf("%s\n", path);
    FILE* fp = fopen(path, "w");

    if (fp == NULL) {
        printf("Error file description.\n");
        return;
    }

    fprintf(fp, "# %s\n\n", windowing->name);
    
    fprintf(fp, "Rootpath: %s\n\n", windowing->rootpath);

    {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(fp, "Test end at: %4d/%02d/%02d, %02d:%02d:%02d.\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }



    fprintf(fp, "\n\n## WSizes\n");
    fprintf(fp, "\n\nNumber:\t%d\n\n", windowing->wsizes.number);
    for (int ws = 0; ws < windowing->wsizes.number - 1; ws++) {
        fprintf(fp, "%d,", windowing->wsizes._[ws]);
    }
    fprintf(fp, "%d\n\n\n", windowing->wsizes._[windowing->wsizes.number - 1]);


    fprintf(fp, "## Captures\n\n");

    fprintf(fp, "Captures number: %d\n\n", windowing->captures.number);

    fprintf(fp, "id,name,capture_type,class,fnreq_max,nmessages,q,r,qr,source\n");
    for (int i = 0; i < windowing->captures.number; i++) {
        Capture* c = &windowing->captures._[i];
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

    fprintf(fp, "Parameters number: %d\n\n", windowing->psets.number);

    fprintf(fp, "id,whitelisting,windowing,infinite_values,nn\n");
    for (int i = 0; i < windowing->psets.number; i++) {
        PSet* p = &windowing->psets._[i];
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