#include <stdio.h>

#include "dn.h"

void write_Pi_file(char path[], Pi* pis, const int N_PIS) {
    FILE *pi_file = fopen(path, "wb");

    fwrite(&N_PIS, sizeof(int), 1, pi_file);

    for (int i = 0; i < N_PIS; ++i) {
        fwrite(&pis[i].infinite_values, sizeof(InfiniteValues), 1, pi_file);
        fwrite(&pis[i].nn, sizeof(NN), 1, pi_file);
        fwrite(&pis[i].whitelisting, sizeof(Whitelisting), 1, pi_file);
        fwrite(&pis[i].windowing, sizeof(WindowingType), 1, pi_file);
        fwrite(&pis[i].wsize, sizeof(int), 1, pi_file);
        fwrite(&pis[i].id, sizeof(int), 1, pi_file);

        if (i == 177) {
            printf("inf: %f\t%f\n", pis[i].infinite_values.ninf, pis[i].infinite_values.pinf);
            printf(" nn: %d\n", pis[i].nn);
            printf("wht: %d\t%f\n", pis[i].whitelisting.rank, pis[i].whitelisting.value);
            printf("win: %d\n", pis[i].windowing);
            printf(" ws: %d\n", pis[i].wsize);
            printf(" id: %d\n\n", pis[i].id);
        }
    }

    fclose(pi_file);
}

void read_Pi_file(char path[], Pi *pi) {
    FILE *pi_file = fopen(path, "rb");
    int _n;

    fread(&_n, sizeof(int), 1, pi_file);

    for (int i = 0; i < _n; ++i) {
        InfiniteValues infinite_values;
        NN nn;
        Whitelisting whitelisting;
        WindowingType windowing;
        int wsize;
        int id;

        fread(&infinite_values, sizeof(InfiniteValues), 1, pi_file);
        fread(&nn, sizeof(NN), 1, pi_file);
        fread(&whitelisting, sizeof(Whitelisting), 1, pi_file);
        fread(&windowing, sizeof(WindowingType), 1, pi_file);
        fread(&wsize, sizeof(int), 1, pi_file);
        fread(&id, sizeof(int), 1, pi_file);

        if (i == 177) {
            printf("inf: %f\t%f\n", infinite_values.ninf, infinite_values.pinf);
            printf(" nn: %d\n", nn);
            printf("wht: %d\t%f\n", whitelisting.rank, whitelisting.value);
            printf("win: %d\n", windowing);
            printf(" ws: %d\n", wsize);
            printf(" id: %d\n\n", id);
        }
    }

    fclose(pi_file);
}