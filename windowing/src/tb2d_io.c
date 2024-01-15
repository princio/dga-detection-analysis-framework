
#include "tb2d_io.h"

// #include "logger.h"
#include "tb2d.h"
#include "tb2w_io.h"

size_t ciao = 0;

void tb2d_io_flag_testbed(IOReadWrite rw, FILE* file, WSize wsize, int line) {
    char flag_code[80];
    sprintf(flag_code, "testbed_%ld", wsize);
    tb2_io_flag(rw, file, flag_code, line);
}

void _tb2d_io_dataset_windows(IOReadWrite rw, FILE* file, RTB2D tb2d, MANY(RWindow) windows) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    for (size_t i = 0; i < windows.number; i++) {
        struct {
            uint32_t idxsource;
            uint32_t windowing_idxwindow;
        } windex;
        memset(&windex, 0, sizeof(windex));

        if (rw == IO_WRITE) {
            windex.idxsource = windows._[i]->windowing->source->index.all;
            windex.windowing_idxwindow = windows._[i]->index;
        }

        FRW(windex);

        if (rw == IO_READ) {
            const size_t idxsource = windex.idxsource;
            const RWindow* windowing_window_ref = &BY_GET(tb2d->tb2w->windowing, source)->windows._[windex.windowing_idxwindow];
            windows._[i] = BY_GET(tb2d->tb2w->windowing, source)->windows._[windex.windowing_idxwindow];
        }

        // LOG_TRACE("ciao#%-10ld\t%-10s\t%8u\t%8u\t%5ld",
        //     ciao++,
        //     rw == IO_WRITE ? "W" : "R",
        //     windex.idxsource,
        //     windex.windowing_idxwindow,
        //     windows._[i]->index
        // );
    }
}

void _tb2d_io_dataset(IOReadWrite rw, FILE* file, RTB2D tb2d, RDataset* dataset_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    Index windows_counter;
    WSize wsize;
    Index walker;

    if (rw == IO_WRITE) {
        windows_counter = dataset_counter(*dataset_ref);
        wsize = (*dataset_ref)->wsize;
    }

    FRW(wsize);
    FRW(windows_counter);

    if (rw == IO_READ) {
        (*dataset_ref) = dataset_create(tb2d->tb2w->wsize, windows_counter);
    }

    _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.all);
    _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.binary[0]);
    _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.binary[1]);
    DGAFOR(cl) {
        _tb2d_io_dataset_windows(rw, file, tb2d, (*dataset_ref)->windows.multi[cl]);
    }

    // memset(&walker, 0, sizeof(Index));
    // for (size_t i = 0; i < windows_counter.all; i++) {
    //     RWindow* windowing_window_ref;
    //     RWindow* window_ref = &(*dataset_ref)->windows.all._[i];
    //     RWindow* window_bin_ref;
    //     RWindow* window_mul_ref;
    //     RSource source;

    //     struct {
    //         uint32_t idxsource;
    //         uint32_t windowing_idxwindow;
    //     } windex;
    //     memset(&windex, 0, sizeof(windex));

    //     if (rw == IO_WRITE) {
    //         windex.idxsource = (*window_ref)->windowing->source->index.all;
    //         windex.windowing_idxwindow = (*window_ref)->index;
    //     }

    //     FRW(windex);

    //     source = tb2d->tb2w->sources._[windex.idxsource];
    //     const size_t idxsource = windex.idxsource;

    //     windowing_window_ref = &BY_GET(tb2d->tb2w->windowing, source)->windows._[windex.windowing_idxwindow];

    //     window_bin_ref = &(*dataset_ref)->windows.binary[source->wclass.bc]._[walker.binary[source->wclass.bc]++];
    //     window_mul_ref = &(*dataset_ref)->windows.multi[source->wclass.mc]._[walker.multi[source->wclass.mc]++];

    //     if (rw == IO_READ) {
    //         const size_t idxsource = windex.idxsource;
    //         (*window_ref) = BY_GET(tb2d->tb2w->windowing, source)->windows._[windex.windowing_idxwindow];
    //         (*window_bin_ref) = (*window_ref);
    //         (*window_mul_ref) = (*window_ref);
    //     }

    //     LOG_TRACE(
    //         "ciao#%-10ld\t%-10s\t%8u\t%8u\t%5ld\t%5ld\t%5ld",
    //         ciao++,
    //         rw == IO_WRITE ? "W" : "R",
    //         windex.idxsource,
    //         windex.windowing_idxwindow,
    //         (*window_ref)->index,
    //         (*window_bin_ref)->index,
    //         (*window_mul_ref)->index
    //     );
    // }
}

void _tb2d_io_datasets(IOReadWrite rw, FILE* file, RTB2D tb2d) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    BY_FOR(*tb2d, try) {

        tb2_io_flag(rw, file, "tb2d-dataset-start", __LINE__);
        _tb2d_io_dataset(rw, file, tb2d, &BY_GET(*tb2d, try).dataset);
        tb2_io_flag(rw, file, "tb2d-dataset-end", __LINE__);

        BY_FOR(*tb2d, fold) {
            tb2_io_flag(rw, file, "tb2d-fold-start", __LINE__);

            if (rw == IO_READ) {
                MANY_INIT(BY_GET2(*tb2d, try, fold).splits, tb2d->folds._[idxfold].k, DatasetSplit);
            }
            
            for (size_t k = 0; k < BY_GET2(*tb2d, try, fold).splits.number; k++) {
                _tb2d_io_dataset(rw, file, tb2d, &BY_GET2(*tb2d, try, fold).splits._[k].train);
                _tb2d_io_dataset(rw, file, tb2d, &BY_GET2(*tb2d, try, fold).splits._[k].test);
            }

            tb2_io_flag(rw, file, "tb2d-fold-end", __LINE__);
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
    }
}

int tb2d_io(IOReadWrite rw, char rootdir[DIR_MAX], RTB2W tb2w, RTB2D* tb2) {
    ciao = 0;
    char fpath[PATH_MAX];
    int direxists = io_direxists(rootdir);
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

    io_path_concat(rootdir, "tb2d.bin", fpath);

    FILE* file;
    file = io_openfile(rw, fpath);
    if (file == NULL) {
        LOG_ERROR("Impossible to %s file <%s>.\n", rw == IO_WRITE ? "write" : "read", fpath);
        return -1;
    }

    LOG_DEBUG("%sing file <%s>.\n", rw == IO_WRITE ? "Writ" : "Read", fpath);

    tb2_io_flag(rw, file, "tb2d-create-start", __LINE__);

    _tb2d_io_create(rw, file, tb2w, tb2);

    tb2_io_flag(rw, file, "tb2d-create-end", __LINE__);

    _tb2d_io_datasets(rw, file, *tb2);

    tb2_io_flag(rw, file, "tb2d-end", __LINE__);

    fclose(file);

    return 0;
}