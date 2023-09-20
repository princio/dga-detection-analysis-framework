#include "persister.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <endian.h>


#define FW32(A) fwrite32((void*) &A, file)
#define FW64(A) fwrite64((void*) &A, file)
#define FR32(A) fread32((void*) &A, file)
#define FR64(A) fread64((void*) &A, file)

#define FW(A) fwriteN((void*) &A, sizeof(A), file)
#define FR(A) freadN((void*) &A, sizeof(A), file)



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
        FILE *file = fopen("/tmp/test.bin", "wb");

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



int persister_write__psets(char path[], PSets pis, int N_PIS) {
    FILE *file = fopen(path, "wb");

    if (!file) {
        perror("write-Pis");
        return 0;
    }

    FW(N_PIS);

    printf("aa %d\n", pis[0].nn);

    for (int i = 0; i < N_PIS; ++i) {

        FW(pis[i].infinite_values);
        FW(pis[i].nn);
        FW(pis[i].whitelisting);
        FW(pis[i].windowing);
        FW(pis[i].wsize);
        FW(pis[i].id);


        if (i == 177) {
            printf("write 177\n");
            printf("inf: %f\t%f\n", pis[i].infinite_values.ninf, pis[i].infinite_values.pinf);
            printf(" nn: %d\n", pis[i].nn);
            printf("wht: %d\t%f\n", pis[i].whitelisting.rank, pis[i].whitelisting.value);
            printf("win: %d\n", pis[i].windowing);
            printf(" ws: %d\n", pis[i].wsize);
            printf(" id: %d\n\n", pis[i].id);
        }
    }

    return fclose(file) == 0;
}

int persister_read__psets(char path[], PSets* pis, int* N_PIS) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("read-Pis");
        return 0;
    }

    int _n;

    FR(_n);

    PSet* _pis = calloc(sizeof(PSet), _n);
    
    *pis = _pis;
    *N_PIS = _n;

    for (int i = 0; i < _n; ++i) {

        FR(_pis[i].infinite_values);
        FR(_pis[i].nn);
        FR(_pis[i].whitelisting);
        FR(_pis[i].windowing);
        FR(_pis[i].wsize);
        FR(_pis[i].id);

        if (1 || i == 177) {
            PSet* pi = &_pis[i];

            printf("%10d\t", i);
            printf("%10d\t", pi->wsize);
            printf("(%10d, %10.5f)\t", pi->whitelisting.rank, pi->whitelisting.value);
            printf("%10d\t", pi->nn);
            printf("%10d\n", pi->windowing);
        }
    }

    return fclose(file) == 0;
}


int persister_write__captures(char path[], WindowingPtr windowing) {
    printf("persister_write_pcap:\t%s", path);

    FILE *file = fopen(path, "wb");

    if (!file) {
        perror("write-pcap");
        return 0;
    }

    const int32_t n_captures = windowing->n_captures;

    printf("write-n_pcap:\t%d", windowing->n_captures);

    FW(n_captures);

    Captures captures = windowing->captures;
    for (int i = 0; i < n_captures; ++i) {
        FW(captures[i].id);
        FW(captures[i].infected_type);
        FW(captures[i].nmessages);
        FW(captures[i].fnreq_max);
        FW(captures[i].q);
        FW(captures[i].qr);
        FW(captures[i].r);

        if (i == 0) {
            printf(" id: %ld\n", captures[i].id);
            printf("inf: %d\n", captures[i].infected_type);
            printf("nms: %ld\n", captures[i].nmessages);
            printf("frm: %ld\n", captures[i].fnreq_max);
            printf("  q: %ld\n", captures[i].q);
            printf(" qr: %ld\n", captures[i].qr);
            printf("  r: %ld\n\n", captures[i].r);
        }
    }

    return fclose(file) == 0;
}

int persister_read__captures(char path[], WindowingPtr windowing) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        perror("read-pcap");
        return 0;
    }

    int n;
    FR(n);
    printf("Reading %d captures from %s...", n, path);

    windowing->n_captures = n;

    Captures captures = windowing->captures;
    for (int i = 0; i < n; ++i) {
        FR(captures[i].id);
        FR(captures[i].infected_type);
        FR(captures[i].nmessages);
        FR(captures[i].fnreq_max);
        FR(captures[i].q);
        FR(captures[i].qr);
        FR(captures[i].r);

        if (i == 0) {
            printf("\n id: %ld\n", captures[i].id);
            printf("inf: %d\n", captures[i].infected_type);
            printf("nms: %ld\n", captures[i].nmessages);
            printf("frm: %ld\n", captures[i].fnreq_max);
            printf("  q: %ld\n", captures[i].q);
            printf(" qr: %ld\n", captures[i].qr);
            printf("  r: %ld\n\n", captures[i].r);
        }
    }

    return fclose(file) == 0;
}

int persister_write__capturewindows(char* rootpath, WindowingPtr windowing, int32_t capture_index) {
    FILE *file;
    CapturePtr capture;
    CaptureWindowings capture_windowings;

    capture = windowing->captures[capture_index];
    capture_windowings = windowing->captures_windowings[capture_index];

    {
        char path[strlen(rootpath) + 50];
        sprintf(path, "%s/pcap_%ld.bin", rootpath, capture->id);
        file = fopen(path, "wb");
        if (!file) {
            perror("write-windows");
            return 0;
        }
        printf("Writing WINDOWS to %s...\n", path);
    }

    for (int w = 0; w < windowing->n_wsizes; ++w) {
        CaptureWindowingPtr capture_windowing = &capture_windowings[w];
        const int32_t wsize = windowing->wsizes[w];
        const int32_t n_windows = capture_windowing->n_windows;

        FW(n_windows);

        for (int r = 0; r < n_windows; ++r) {
            Window* window = &capture_windowing->windows[r];

            FW(window->wnum);

            // int rr = (capture_windowing->nwindows-8) > 0 ? (windowing->nwindows-8) : 0;
            // if (w == 0 && (r == rr || r == 0)) {
                // printf("   r: %d\n", r);
                // printf("wnum: %d\n", window->wnum);
                // printf(" wsz: %d\n", window->wsize);
            // }

            for (int m = 0; m < window->nmetrics; ++m) {
                WindowMetrics* metrics = &window->metrics[m];

                FW(metrics->pi_id);
                FW(metrics->dn_bad_05);
                FW(metrics->dn_bad_09);
                FW(metrics->dn_bad_099);
                FW(metrics->dn_bad_0999);
                FW(metrics->logit);
                FW(metrics->wcount);

                // printf("[%d]: %f [%d/%d]\n", r, metrics->logit, metrics->whitelistened, window->wsize);

                // if (w == 0 && (r == rr || r == 0) && m == 0) {
                    // printf("  05: %d\n", metrics->dn_bad_05);
                    // printf("  09: %d\n", metrics->dn_bad_09);
                    // printf(" 099: %d\n", metrics->dn_bad_099);
                    // printf("0999: %d\n", metrics->dn_bad_0999);
                    // printf(" lgt: %f\n", metrics->logit);
                    // printf("wcnt: %d\n\n", metrics->wcount);
                // }
            }
        }
    }

    fclose(file);

    return 1;
}


int persister_read__capturewindows(char* rootpath, WindowingPtr windowing, int32_t capture_index) {

    FILE *file;
    CapturePtr capture;
    CaptureWindowings capture_windowings;

    capture = windowing->captures[capture_index];
    capture_windowings = windowing->captures_windowings[capture_index];

    {
        char path[strlen(rootpath) + 50];
        sprintf(path, "%s/pcap_%ld.bin", rootpath, capture->id);
        file = fopen(path, "rb");
        if (!file) {
            perror("read-windows");
            return 0;
        }
        printf("Writing WINDOWS to %s...\n", path);
    }


    for (int w = 0; w < windowing->n_wsizes; ++w) {
        CaptureWindowingPtr capture_windowing = &capture_windowings[w];

        FR(capture_windowing->n_windows);

        const int32_t n_windows = capture_windowing->n_windows;
        for (int r = 0; r < n_windows; ++r) {
            Window* window = &capture_windowing->windows[r];

            FR(window->wnum);

            // int rr = (windowing->nwindows-8) > 0 ? (windowing->nwindows-8) : 0;
            // if (w == 0 && (r == rr || r == 0)) {
                // printf("   r: %d\n", r);
                // printf("wnum: %d\n", window->wnum);
                // printf(" wsz: %d\n", window->wsize);
            // }

            for (int m = 0; m < window->nmetrics; ++m) {
                WindowMetrics* metrics = &window->metrics[m];

                FR(metrics->pi_id);
                FR(metrics->dn_bad_05);
                FR(metrics->dn_bad_09);
                FR(metrics->dn_bad_099);
                FR(metrics->dn_bad_0999);
                FR(metrics->logit);
                FR(metrics->wcount);

                // if (w == 0 && (r == rr || r == 0) && m == 0) {
                    // printf("  05: %d\n", metrics->dn_bad_05);
                    // printf("  09: %d\n", metrics->dn_bad_09);
                    // printf(" 099: %d\n", metrics->dn_bad_099);
                    // printf("0999: %d\n", metrics->dn_bad_0999);
                    // printf(" lgt: %f\n", metrics->logit);
                    // printf("wcnt: %d\n", metrics->wcount);
                // }
            }
        }
    }
    
    fclose(file);

    return 1;
}