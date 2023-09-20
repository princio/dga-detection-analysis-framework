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



int persister_write__psets(WindowingPtr windowing) {
    FILE *file;
    int32_t number;
    PSet* psets;

    {
        char path[500 + 50];
        sprintf(path, "%s/parameters.bin", windowing->rootpath);

        FILE *file = fopen(path, "wb");

        if (!file) {
            perror("write-Pis");
            return 0;
        }
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


        if (i == 177) {
            PSet* pi = &psets[i];
            printf("write 177\n");
            printf("inf: %f\t%f\n", pi->infinite_values.ninf, pi->infinite_values.pinf);
            printf(" nn: %d\n", pi->nn);
            printf("wht: %d\t%f\n", pi->whitelisting.rank, pi->whitelisting.value);
            printf("win: %d\n", pi->windowing);
            printf(" id: %d\n\n", pi->id);
        }
    }

    return fclose(file) == 0;
}

int persister_read__psets(WindowingPtr windowing) {
    FILE *file;
    
    {
        char path[500 + 50];
        sprintf(path, "%s/parameters.bin", windowing->rootpath);

        FILE *file = fopen(path, "rb");
        if (!file) {
            perror("read-Pis");
            return 0;
        }
    }

    FR(windowing->psets.number);

    windowing->psets._ = calloc(sizeof(PSet), windowing->psets.number);
    
    for (int i = 0; i < windowing->psets.number; ++i) {

        FR(windowing->psets._[i].infinite_values);
        FR(windowing->psets._[i].nn);
        FR(windowing->psets._[i].whitelisting);
        FR(windowing->psets._[i].windowing);
        FR(windowing->psets._[i].id);

        if (1 || i == 177) {
            PSet* pi = &windowing->psets._[i];
            printf("read 177\n");
            printf("inf: %f\t%f\n", pi->infinite_values.ninf, pi->infinite_values.pinf);
            printf(" nn: %d\n", pi->nn);
            printf("wht: %d\t%f\n", pi->whitelisting.rank, pi->whitelisting.value);
            printf("win: %d\n", pi->windowing);
            printf(" id: %d\n\n", pi->id);
        }
    }

    return fclose(file) == 0;
}


int persister_write__captures(WindowingPtr windowing) {
    FILE *file;
    int32_t number;
    Captures* captures;
    
    {
        char path[500 + 50];
        sprintf(path, "%s/captures.bin", windowing->rootpath);

        printf("persister_write_pcap:\t%s", path);

        file = fopen(path, "wb");

        if (!file) {
            perror("write-pcap");
            return 0;
        }
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

        if (i == 0) {
            printf(" id: %ld\n", captures->_[i].id);
            printf("inf: %d\n", captures->_[i].class);
            printf("nms: %ld\n", captures->_[i].nmessages);
            printf("frm: %ld\n", captures->_[i].fnreq_max);
            printf("  q: %ld\n", captures->_[i].q);
            printf(" qr: %ld\n", captures->_[i].qr);
            printf("  r: %ld\n\n", captures->_[i].r);
        }
    }

    return fclose(file) == 0;
}

int persister_read__captures(WindowingPtr windowing) {
    FILE *file;
    int32_t number;
    Captures* captures;

    {
        char path[500 + 50];
        sprintf(path, "%s/captures.bin", windowing->rootpath);
        
        FILE *file = fopen(path, "rb");

        if (!file) {
            perror("read-pcap");
            return 0;
        }
    }

    FR(number);

    windowing->captures.number = number;

    Captures* captures = &windowing->captures;
    for (int i = 0; i < number; ++i) {
        FR(captures->_[i].id);
        FR(captures->_[i].class);
        FR(captures->_[i].nmessages);
        FR(captures->_[i].fnreq_max);
        FR(captures->_[i].q);
        FR(captures->_[i].qr);
        FR(captures->_[i].r);

        if (i == 0) {
            printf("\n id: %ld\n", captures->_[i].id);
            printf("inf: %d\n", captures->_[i].class);
            printf("nms: %ld\n", captures->_[i].nmessages);
            printf("frm: %ld\n", captures->_[i].fnreq_max);
            printf("  q: %ld\n", captures->_[i].q);
            printf(" qr: %ld\n", captures->_[i].qr);
            printf("  r: %ld\n\n", captures->_[i].r);
        }
    }

    return fclose(file) == 0;
}

int persister_write__capturewsets(WindowingPtr windowing, int32_t capture_index) {


    FILE *file;
    CapturePtr capture;
    CaptureWSets capture_wsets;

    {
        char path[500 + 50];
        sprintf(path, "%s/captures_%d.bin", windowing->rootpath, capture_index);

        file = fopen(path, "wb");
        if (!file) {
            perror("write-windows");
            return 0;
        }
        printf("Writing WINDOWS to %s...\n", path);
    }

    capture = &windowing->captures._[capture_index];
    capture_wsets = windowing->captures_wsets[capture_index];

    for (int w = 0; w < windowing->wsizes.number; ++w) {
        WSetPtr capture_windowing = &capture_wsets[w];
        const int32_t wsize = windowing->wsizes._[w];
        const int32_t n_windows = capture_windowing->number;

        FW(n_windows);

        for (int r = 0; r < n_windows; ++r) {
            Window* window = &capture_windowing->_[r];

            FW(window->wnum);

            // int rr = (capture_windowing->nwindows-8) > 0 ? (windowing->nwindows-8) : 0;
            // if (w == 0 && (r == rr || r == 0)) {
                // printf("   r: %d\n", r);
                // printf("wnum: %d\n", window->wnum);
                // printf(" wsz: %d\n", window->wsize);
            // }

            for (int m = 0; m < window->nmetrics; ++m) {
                WindowMetricSet* metric = &window->metrics->_[m];

                FW(metric->pi_id);
                FW(metric->dn_bad_05);
                FW(metric->dn_bad_09);
                FW(metric->dn_bad_099);
                FW(metric->dn_bad_0999);
                FW(metric->logit);
                FW(metric->wcount);

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


int persister_read__capturewsets(WindowingPtr windowing, int32_t capture_index) {

    FILE *file;
    CapturePtr capture;
    CaptureWSets capture_wsets;

    {
        char path[500 + 50];
        sprintf(path, "%s/captures_%d.bin", windowing->rootpath, capture_index);

        file = fopen(path, "rb");
        if (!file) {
            perror("read-windows");
            return 0;
        }
        printf("Writing WINDOWS to %s...\n", path);
    }

    capture = &windowing->captures._[capture_index];
    capture_wsets = windowing->captures_wsets[capture_index];


    for (int w = 0; w < windowing->wsizes.number; ++w) {
        CaptureWSets capture_windowing = &capture_wsets[w];

        FR(capture_windowing->number);

        const int32_t n_windows = capture_windowing->number;
        for (int r = 0; r < n_windows; ++r) {
            Window* window = &capture_windowing->_[r];

            FR(window->wnum);

            // int rr = (windowing->nwindows-8) > 0 ? (windowing->nwindows-8) : 0;
            // if (w == 0 && (r == rr || r == 0)) {
                // printf("   r: %d\n", r);
                // printf("wnum: %d\n", window->wnum);
                // printf(" wsz: %d\n", window->wsize);
            // }

            for (int m = 0; m < window->nmetrics; ++m) {
                WindowMetricSet* metric = &window->metrics->_[m];

                FR(metric->pi_id);
                FR(metric->dn_bad_05);
                FR(metric->dn_bad_09);
                FR(metric->dn_bad_099);
                FR(metric->dn_bad_0999);
                FR(metric->logit);
                FR(metric->wcount);

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