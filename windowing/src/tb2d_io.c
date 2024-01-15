
#include "tb2d_io.h"

// #include "logger.h"
#include "tb2d.h"
#include "tb2w_io.h"

void tb2d_io_flag_testbed(IOReadWrite rw, FILE* file, WSize wsize, int line) {
    char flag_code[80];
    sprintf(flag_code, "testbed_%ld", wsize);
    tb2_io_flag(rw, file, flag_code, line);
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

    memset(&walker, 0, sizeof(Index));
    for (size_t i = 0; i < windows_counter.all; i++) {
        RWindow* window_ref = &(*dataset_ref)->windows.all._[i];
        RWindow* window_bin_ref;
        RWindow* window_mul_ref;
        RSource source;

        struct {
            uint32_t windowing_idxwindow;
            uint32_t idxsource;
        } windex;
        memset(&windex, 0, sizeof(windex));

        if (rw == IO_WRITE) {
            windex.windowing_idxwindow = (*window_ref)->index;
            windex.idxsource = (*window_ref)->windowing->source->index.all;
        }

        FRW(windex);

        source = tb2d->tb2w->sources._[windex.idxsource];

        window_bin_ref = &(*dataset_ref)->windows.binary[source->wclass.bc]._[walker.binary[source->wclass.bc]++];
        window_mul_ref = &(*dataset_ref)->windows.multi[source->wclass.mc]._[walker.multi[source->wclass.mc]++];

        if (rw == IO_READ) {
            const size_t idxsource = windex.idxsource;
            (*window_ref) = BY_GET(tb2d->tb2w->windowing, source)->windows._[windex.windowing_idxwindow];
            (*window_bin_ref) = (*window_ref);
            (*window_mul_ref) = (*window_ref);
        }
    }
}

// void tb2d_io_all_dataset(IOReadWrite rw, FILE* file, RTB2W tb2) {
//     FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

//     FRW(tb2->dataset.n.try);
//     FRW(tb2->dataset.n.fold);

//     if (rw == IO_READ) {
//         MANY_INIT(tb2->dataset.folds, tb2->dataset.n.fold, FoldConfig);
//     }

//     for (size_t idxfold = 0; idxfold < tb2->dataset.folds.number; idxfold++) {
//         FRW(tb2->dataset.folds._[idxfold].k);
//         FRW(tb2->dataset.folds._[idxfold].k_test);
//     }

//     BY_FOR(tb2->dataset, try) {
//         tb2d_io_dataset(rw, file, tb2, &BY_GET(tb2->dataset, try).dataset);
//         if (rw == IO_READ) {
//             BY_INIT(tb2->dataset, BY_GET(tb2->dataset, try), fold, TestBed2DatasetBy);
//         }
//         BY_FOR(tb2->dataset, fold) {
//             FRW(BY_GET2(tb2->dataset, try, fold).isok);
//             // if (0 == BY_GET3(tb2->dataset, wsize, try, fold).isok) {
//             //     continue;
//             // }
//             if (rw == IO_READ) {
//                 MANY_INIT(BY_GET2(tb2->dataset, try, fold).splits, tb2->dataset.folds._[idxfold].k, DatasetSplit);
//             }
//             for (size_t k = 0; k < BY_GET2(tb2->dataset, try, fold).splits.number; k++) {
//                 tb2d_io_dataset(rw, file, tb2, &BY_GET2(tb2->dataset, try, fold).splits._[k].train);
//                 tb2d_io_dataset(rw, file, tb2, &BY_GET2(tb2->dataset, try, fold).splits._[k].test);
//             }
//         }
//     }
// }

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
    char fpath[PATH_MAX];

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