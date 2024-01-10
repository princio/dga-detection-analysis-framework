
#include "main_windowing.h"

#include "tb2w_io.h"
#include "stratosphere.h"

int main_windowing_generate(char dirpath[DIR_MAX], const WSize wsize, const size_t max_sources_number, ParameterGenerator pg) {
    RTB2W tb2w;

    if (io_direxists(dirpath)) {
        return -1;
    }

    {
        printf("Performing windowing...\n");
        tb2w = tb2w_create(dirpath, wsize, pg);

        stratosphere_add(tb2w, max_sources_number);

        if (max_sources_number && tb2w->sources.number < max_sources_number) {
            printf("Error: max sources number not correspond.\n");
            exit(-1);
        }

        tb2w_windowing(tb2w);

        printf("Applying for %ld configs.\n",  tb2w->configsuite.configs.number);
        tb2w_apply(tb2w);
    }

    if (io_makedirs(dirpath)) {
        return -1;
    }

    tb2w_io(IO_WRITE, dirpath, &tb2w);

    tb2w_free(tb2w);

    return 0;
}

RTB2W main_windowing_load(char dirpath[DIR_MAX]) {
    RTB2W tb2w;

    tb2w_io(IO_READ, dirpath, &tb2w);

    return tb2w;
}