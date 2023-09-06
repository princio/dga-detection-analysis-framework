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


int write_Pi_file(char path[], Pi* pis, int N_PIS) {
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

int read_Pi_file(char path[], Pi** pis, int* N_PIS) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("read-Pis");
        return 0;
    }

    int _n;

    FR(_n);

    Pi* _pis = calloc(sizeof(Pi), _n);
    
    *pis = _pis;
    *N_PIS = _n;

    for (int i = 0; i < _n; ++i) {

        FR(_pis[i].infinite_values);
        FR(_pis[i].nn);
        FR(_pis[i].whitelisting);
        FR(_pis[i].windowing);
        FR(_pis[i].wsize);
        FR(_pis[i].id);

        if (i == 177) {
            printf("read 177\n");
            printf("inf: %f\t%f\n", _pis[i].infinite_values.ninf, _pis[i].infinite_values.pinf);
            printf(" nn: %d\n",_pis[i].nn);
            printf("wht: %d\t%f\n", _pis[i].whitelisting.rank, _pis[i].whitelisting.value);
            printf("win: %d\n", _pis[i].windowing);
            printf(" ws: %d\n", _pis[i].wsize);
            printf(" id: %d\n\n", _pis[i].id);
        }
    }

    return fclose(file) == 0;
}


int write_PCAP_file(char path[], PCAP* pcaps, int N_PCAP) {
    printf("write_PCAP_file:\t%s", path);

    FILE *file = fopen(path, "wb");

    if (!file) {
        perror("write-pcap");
        return 0;
    }

    printf("write-n_pcap:\t%d", N_PCAP);

    FW(N_PCAP);

    for (int i = 0; i < N_PCAP; ++i) {
        FW(pcaps[i].id);
        FW(pcaps[i].infected);
        FW(pcaps[i].nmessages);
        FW(pcaps[i].q);
        FW(pcaps[i].qr);
        FW(pcaps[i].r);

        if (i == 0) {
            printf(" id: %ld\n", pcaps[i].id);
            printf("inf: %d\n", pcaps[i].infected);
            printf("nms: %ld\n", pcaps[i].nmessages);
            printf("  q: %ld\n", pcaps[i].q);
            printf(" qr: %ld\n", pcaps[i].qr);
            printf("  r: %ld\n\n", pcaps[i].r);
        }
    }

    return fclose(file) == 0;
}

int read_PCAP_file(char path[], PCAP** pcaps, int* N_PCAP) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        perror("read-pcap");
        return 0;
    }

    int n;
    FR(n);

    PCAP* _pcaps = calloc(n, sizeof(PCAP));;
    *N_PCAP = n;

    printf("Reading %d pcaps from %s...", *N_PCAP, path);
    *pcaps = _pcaps;

    for (int i = 0; i < n; ++i) {
        FR(_pcaps[i].id);
        FR(_pcaps[i].infected);
        FR(_pcaps[i].nmessages);
        FR(_pcaps[i].q);
        FR(_pcaps[i].qr);
        FR(_pcaps[i].r);

        if (i == 0) {
            printf("\n id: %ld\n", _pcaps[i].id);
            printf("inf: %d\n", _pcaps[i].infected);
            printf("nms: %ld\n", _pcaps[i].nmessages);
            printf("  q: %ld\n", _pcaps[i].q);
            printf(" qr: %ld\n", _pcaps[i].qr);
            printf("  r: %ld\n\n", _pcaps[i].r);
        }
    }

    return fclose(file) == 0;
}