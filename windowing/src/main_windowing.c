
#include "main_windowing.h"

#include "tb2w_io.h"
#include "stratosphere.h"

#include <assert.h>


RTB2W main_windowing_generate(char dirpath[DIR_MAX], const WSize wsize, const size_t max_sources_number, ParameterGenerator pg) {
    RTB2W tb2w;

    {
        printf("Performing windowing...\n");
        tb2w = tb2w_create(dirpath, wsize, pg);

        stratosphere_add(tb2w, max_sources_number);

        if (max_sources_number && tb2w->sources.number < max_sources_number) {
            printf("Error: max sources number not correspond.\n");
            exit(-1);
        }

        tb2w_windowing(tb2w);

        if (tb2w_io(IO_WRITE, dirpath, &tb2w)) {
            printf("Impossible to save TB2W at: %s\n", dirpath);
            return NULL;
        }

        printf("Applying for %ld configs.\n",  tb2w->configsuite.configs.number);
        tb2w_apply(tb2w);
    }

    return tb2w;
}

RTB2W main_windowing_load(char dirpath[DIR_MAX]) {
    RTB2W tb2w;

    if (tb2w_io(IO_READ, dirpath, &tb2w)) {
        LOG_WARN("Impossible to load TB2W at: %s.", dirpath);
        return NULL;
    }

    return tb2w;
}

int main_windowing_test(char dirpath[DIR_MAX], const WSize wsize, const size_t max_sources_number, ParameterGenerator pg) {
    RTB2W tb2w_generated;
    RTB2W tb2w_loaded;

    io_path_concat(dirpath, "test/", dirpath);

    tb2w_generated = main_windowing_generate(dirpath, wsize, max_sources_number, pg);
    if (NULL == tb2w_generated) {
        exit(-1);
    }
    tb2w_loaded = main_windowing_load(dirpath);
    if (NULL == tb2w_loaded) {
        exit(-1);
    }

    #define _TEST(A) printf("%100s -> ", #A); puts(A ? "success." : "failed."); if (!(A)) return -1;

    _TEST(tb2w_generated->sources.number == tb2w_loaded->sources.number);
    _TEST(tb2w_generated->configsuite.configs.number == tb2w_loaded->configsuite.configs.number);
    _TEST(tb2w_generated->wsize == tb2w_loaded->wsize);

    size_t diffs = 0;
    for (size_t idxsource = 0; idxsource < tb2w_generated->sources.number; idxsource++) {
        for (size_t idxwindow = 0; idxwindow < tb2w_generated->windowing.bysource._[idxsource]->windows.number; idxwindow++) {
            for (size_t idxconfig = 0; idxconfig < tb2w_generated->configsuite.configs.number; idxconfig++) {
                WApply* wa1;
                WApply* wa2;

                wa1 = &tb2w_generated->windowing.bysource._[idxsource]->windows._[idxwindow]->applies._[idxconfig];
                wa2 = &tb2w_loaded->windowing.bysource._[idxsource]->windows._[idxwindow]->applies._[idxconfig];

                size_t diff = (wa1->logit - wa2->logit) * 10e12;
                
                diffs += (diff != 0);
            }
        }
    }
    _TEST(diffs == 0);

    #undef _TEST

    tb2w_free(tb2w_generated);
    tb2w_free(tb2w_loaded);

    return 0;
}