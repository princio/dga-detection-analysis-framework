
#include "dataset.h"

#include "windows.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

Index dataset_count_sources(Dataset ds) {
    int32_t sources[MAX_Sources];
    Index count_sources;

    memset(&count_sources, 0, sizeof(Index));

    DGAFOR(cl) {
        for (int32_t i = 0; i < ds[cl].number; i++)
        {
            for (int32_t w = 0; w < ds[cl].number; w++) {
                const int32_t all_index = ds[cl]._[w]->source_index.all;
                if (sources[all_index] == 0) {
                    count_sources.all++;
                    count_sources.binary[ds[cl]._[w]->dgaclass > 0]++;
                    count_sources.multi[ds[cl]._[w]->dgaclass]++;
                    sources[all_index] = 1;
                }
            }
        }
    }

    return count_sources;
}