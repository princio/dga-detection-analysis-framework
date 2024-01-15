
#include "main_dataset.h"

#include "tb2d_io.h"

RTB2D main_dataset_generate(char rootpath[DIR_MAX], RTB2W tb2w ,const size_t n_try, MANY(FoldConfig) foldconfig_many) {
    RTB2D tb2d;

    tb2d = tb2d_create(tb2w, n_try, foldconfig_many);

    tb2d_run(tb2d);

    tb2d_io(IO_WRITE, rootpath, NULL, &tb2d);

    return tb2d;
}

RTB2D main_dataset_load(char dirpath[DIR_MAX], RTB2W tb2w) {
    RTB2D tb2d;

    tb2d_io(IO_READ, dirpath, tb2w, &tb2d);

    return tb2d;
}