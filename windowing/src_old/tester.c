#include "tester.h"

#include <stdarg.h>
#include <stdio.h>

static FILE* fp;
static FILE* fp2;

void tester_init() {
    fp = fopen("./ciao.csv", "w");
    fprintf(fp, "try,ci,wn,ws,id,nr,");
    for (int i = 0; i < 15; i++) {
        fprintf(fp, "hex%d,", i);
    }
    fprintf(fp, "hex\n");


    fp2 = fopen("./ciao2.csv", "w");
    fprintf(fp2, "ci,wn,ws,hex0,hex1\n");
}

void tester_close() {
    fclose(fp);
    fclose(fp2);
}

void tester_fprintf(const char * format, ...) {
    va_list args;
    va_start (args, format);
    vfprintf (fp, format, args);
    va_end (args);
}

void tester_stratosphere_hex(int trys, int capture_index, int wnum, int ws, int id, int r, WindowMetricSets metrics) {
    int S = sizeof(WindowMetricSet) * metrics.number;

    fprintf(fp, "%d,%d,%d,%d,%d,%d,", trys, capture_index, wnum, ws, id, r);
    uint8_t* a = (uint8_t*) metrics._;
    for (int q = 0; q < S/10; ++q) {
        fprintf(fp, "%02X", a[q]);
    }
    fprintf(fp, "\n");
}

void tester_stratosphere(int trys, int capture_index, int wnum, int ws, int id, int r, WindowMetricSets metrics) {
    return;
    fprintf(fp, "%d,%d,%d,%d,%d,%d,", trys, capture_index, wnum, ws, id, r);
    // for (int q = 0; q < metrics.number; ++q) {
    //     fprintf(fp, "%d ", metrics._[q].wcount);
    // }

    // for (int m = 0; m < metrics.number; ++m) {
    //     for (int q = 0; q < 8; ++q) {
    //         fprintf(fp, "%02X", ((uint8_t*) &metrics._[m].logit)[q]);
    //     }
    //     if (m < metrics.number - 1) fprintf(fp, " ");
    // }

    for (int m = 0; m < metrics.number; ++m) {
        for (size_t q = 0; q < sizeof(WindowMetricSet); ++q) {
            fprintf(fp, "%02X", ((uint8_t*) &metrics._[m])[q]);
        }
        fprintf(fp, ",");
    }


    for (size_t q = 0; q < metrics.number * sizeof(WindowMetricSet); ++q) {
        fprintf(fp, "%02X", ((uint8_t*) metrics._)[q]);
    }

    fprintf(fp, "\n");
}



void tester_comparer(int capture_index, int wn, int ws, WindowMetricSets metrics0, WindowMetricSets metrics1) {
    return;
    fprintf(fp2, "%d,%d,%d,", capture_index, wn, ws);
    // for (int q = 0; q < metrics.number; ++q) {
    //     fprintf(fp, "%d ", metrics._[q].wcount);
    // }

    // for (int m = 0; m < metrics.number; ++m) {
    //     for (int q = 0; q < 8; ++q) {
    //         fprintf(fp, "%02X", ((uint8_t*) &metrics._[m].logit)[q]);
    //     }
    //     if (m < metrics.number - 1) fprintf(fp, " ");
    // }


    for (size_t q = 0; q < metrics0.number * sizeof(WindowMetricSet); ++q) {
        fprintf(fp2, "%02X", ((uint8_t*) metrics0._)[q]);
    }
    fprintf(fp2, ",");


    for (size_t q = 0; q < metrics1.number * sizeof(WindowMetricSet); ++q) {
        fprintf(fp2, "%02X", ((uint8_t*) metrics1._)[q]);
    }

    fprintf(fp2, "\n");
}