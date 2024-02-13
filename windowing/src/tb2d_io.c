
#include "tb2d_io.h"

// #include "logger.h"
#include "tb2d.h"
#include "tb2w_io.h"

size_t ciao = 0;

void tb2d_io_flag_testbed(IOReadWrite rw, FILE* file, WSize wsize, int line) {
    char flag_code[80];
    sprintf(flag_code, "testbed_%ld", wsize);
    io_flag(rw, file, flag_code, line);
}

void _tb2d_io_dataset_windows(IOReadWrite rw, FILE* file, RTB2D tb2d, MANY(RWindow) windows) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    struct __windex {
        uint32_t idxsource;
        uint32_t windowing_idxwindow;
    };

    struct __windex* windexes = calloc(windows.number, sizeof(struct __windex));

    if (rw == IO_WRITE) {
        for (size_t i = 0; i < windows.number; i++) {
            windexes[i].idxsource = windows._[i]->windowing->source->index.all;
            windexes[i].windowing_idxwindow = windows._[i]->index;
        }
    }

    FRW_SIZE((*windexes), windows.number * sizeof(struct __windex));

    if (rw == IO_READ) {
        for (size_t i = 0; i < windows.number; i++) {
            const size_t idxsource = windexes[i].idxsource;
            const RWindow* windowing_window_ref = &BY_GET(tb2d->tb2w->windowing, source)->windows._[windexes[i].windowing_idxwindow];
            windows._[i] = BY_GET(tb2d->tb2w->windowing, source)->windows._[windexes[i].windowing_idxwindow];
        }
    }

    free(windexes);
}

void _tb2d_io_dataset(IOReadWrite rw, FILE* file, RTB2D tb2d, RDataset* dataset_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    Index windows_counter;
    WSize wsize;
    size_t n_configs;
    Index walker;

    if (rw == IO_WRITE) {
        n_configs = (*dataset_ref)->n_configs;
        wsize = (*dataset_ref)->wsize;
        windows_counter = dataset_counter(*dataset_ref);
    }

    FRW(n_configs);
    FRW(wsize);
    FRW(windows_counter);

    if (rw == IO_READ) {
        (*dataset_ref) = dataset_create(tb2d->tb2w->wsize, windows_counter, n_configs);
    }

    for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
        FRW((*dataset_ref)->minmax._[idxconfig].min);
        FRW((*dataset_ref)->minmax._[idxconfig].max);
    }

    _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.all);
    _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.binary[0]);
    _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.binary[1]);
    DGAFOR(cl) {
        _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.multi[cl]);
    }
}

void _tb2d_io_datasets(IOReadWrite rw, FILE* file, RTB2D tb2d) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    BY_FOR(*tb2d, try) {

        io_flag(rw, file, "tb2d-dataset-start", __LINE__);
        _tb2d_io_dataset(rw, file, tb2d, &BY_GET(*tb2d, try).dataset);
        io_flag(rw, file, "tb2d-dataset-end", __LINE__);

        BY_FOR(*tb2d, fold) {
            io_flag(rw, file, "tb2d-fold-start", __LINE__);
            DatasetSplits* splits = &BY_GET2(*tb2d, try, fold);

            if (rw == IO_READ) {
                MANY_INIT(splits->splits, tb2d->folds._[idxfold].k, DatasetSplit);
            }
            
            splits->isok = 1;
            for (size_t k = 0; k < splits->splits.number; k++) {
                _tb2d_io_dataset(rw, file, tb2d, &splits->splits._[k].train);
                _tb2d_io_dataset(rw, file, tb2d, &splits->splits._[k].test);

                if (splits->splits._[k].train->windows.binary[0].number == 0 || splits->splits._[k].train->windows.binary[1].number == 0) {
                    splits->isok = 0;
                }
            }

            io_flag(rw, file, "tb2d-fold-end", __LINE__);

            LOG_TRACE("%s:\tTry=%ld\tFold=%ld", rw == IO_WRITE ? "Written" : "Read", idxtry, idxfold);
        }
    }
}

void _tb2d_io_create(IOReadWrite rw, FILE* file, RTB2W tb2w, RTB2D* tb2d) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    MANY(FoldConfig) foldconfigs;
    size_t n_tries;

    if (rw == IO_WRITE) {
        n_tries = (*tb2d)->n.try;
        foldconfigs = (*tb2d)->folds;
    }

    FRW(n_tries);
    FRW(foldconfigs.number);
    
    if (rw == IO_READ) {
        MANY_INIT(foldconfigs, foldconfigs.number, FoldConfig);
    }

    for (size_t idxfoldconfig = 0; idxfoldconfig < foldconfigs.number; idxfoldconfig++) {
        FRW(foldconfigs._[idxfoldconfig]);
    }

    if (rw == IO_READ) {
        *tb2d = tb2d_create(tb2w, n_tries, foldconfigs);

        MANY_FREE(foldconfigs);
    }
}

int tb2d_io(IOReadWrite rw, char rootdir[DIR_MAX], RTB2W tb2w, RTB2D* tb2) {
    ciao = 0;
    char fpath[PATH_MAX];

    {
        const int direxists = io_direxists(rootdir);
        if (rw == IO_WRITE) {
            if (!direxists && io_makedirs(rootdir)) {
                LOG_ERROR("Impossible to save file because is impossible to create the directory: %s", rootdir);
                return -1;
            } 
        } else
        if (!direxists) {
            LOG_ERROR("Impossible to read file because directory not exists: %s", rootdir);
            return -1;
        }
    }

    io_path_concat(rootdir, "tb2d.bin", fpath);

    FILE* file;
    file = io_openfile(rw, fpath);
    if (file == NULL) {
        LOG_ERROR("Impossible to %s file <%s>.\n", rw == IO_WRITE ? "write" : "read", fpath);
        return -1;
    }

    LOG_DEBUG("%sing file <%s>.\n", rw == IO_WRITE ? "Writ" : "Read", fpath);

    io_flag(rw, file, "tb2d-create-start", __LINE__);

    _tb2d_io_create(rw, file, tb2w, tb2);

    io_flag(rw, file, "tb2d-create-end", __LINE__);

    _tb2d_io_datasets(rw, file, *tb2);

    io_flag(rw, file, "tb2d-end", __LINE__);

    fclose(file);

    return 0;
}