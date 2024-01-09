
#include "main_dataset.h"

int main_dataset_generate(char dirname[DIR_MAX], RTB2W tb2w ,const size_t n_try, MANY(FoldConfig) foldconfig_many) {
    char dirpath[DIR_MAX];
    RTB2D tb2d;

    {
        char __name[100];
        io_path_concat(tb2w->rootdir, dirname, dirpath);
        if (io_makedirs_notoverwrite(dirpath)) {
            return -1;
        }
    }

    MANY(FoldConfig) foldconfigs_many;
    FoldConfig foldconfigs[] = {
        { .k = 10, .k_test = 2 },
        { .k = 10,.k_test = 4, },
        { .k = 10,.k_test = 6, },
        { .k = 10,.k_test = 8, },
    };

    MANY_INIT(foldconfigs_many, sizeof(foldconfigs) / sizeof(FoldConfig), FoldConfig);

    tb2d = tb2d_generate(tb2w, n_try, foldconfigs_many);

    // testbed_dataset_io();

    tb2d_free(tb2d);

    return 0;
}

RTB2D main_dataset_load(RTB2W tb2w, char dirname[DIR_MAX]) {
    char dirpath[DIR_MAX];
    RTB2D tb2d;

    {
        char __name[100];
        io_path_concat(tb2w->rootdir, dirname, dirpath);
        if (io_makedirs_notoverwrite(dirpath)) {
            return NULL;
        }
    }

    // tb2d_io(IO_READ, dirpath, &tb2d);

    return NULL;
}