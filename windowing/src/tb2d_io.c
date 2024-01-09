
#include "tb2d_io.h"

#include "logger.h"
#include "tb2w_io.h"

void tb2d_io_flag_testbed(IOReadWrite rw, FILE* file, WSize wsize, int line) {
    char flag_code[80];
    sprintf(flag_code, "testbed_%ld", wsize);
    tb2_io_flag(rw, file, flag_code, line);
}

void tb2d_io_dataset(IOReadWrite rw, FILE* file, RTB2W tb2, RDataset* dataset_ref) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    Index windows_counter;
    size_t idxwsize;
    Index walker;

    if (rw == IO_WRITE) {
        windows_counter = dataset_counter(*dataset_ref);
        idxwsize = (*dataset_ref)->wsize;
    }

    FRW(idxwsize);
    FRW(windows_counter);

    if (rw == IO_READ) {
        (*dataset_ref) = dataset_create(tb2->wsize, windows_counter);
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

        source = tb2->sources._[windex.idxsource];

        window_bin_ref = &(*dataset_ref)->windows.binary[source->wclass.bc]._[walker.binary[source->wclass.bc]++];
        window_mul_ref = &(*dataset_ref)->windows.multi[source->wclass.mc]._[walker.multi[source->wclass.mc]++];

        if (rw == IO_READ) {
            const size_t idxsource = windex.idxsource;
            (*window_ref) = BY_GET(tb2->windowing, source)->windows._[windex.windowing_idxwindow];
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

int tb2d_io(IOReadWrite rw, char rootdir[DIR_MAX], RTB2D* tb2) {
    char fpath[PATH_MAX];

    io_path_concat(rootdir, "tb2d.bin", fpath);

    FILE* file;
    file = io_openfile(rw, fpath);
    if (file == NULL) {
        LOG_DEBUG("[%s] impossible to open file <%s>.", rw == IO_WRITE ? "Writing" : "Reading", rootdir);
        return -1;
    }

    LOG_TRACE("%s from file %s.", rw == IO_WRITE ? "Writing" : "Reading", rootdir);

    tb2d_io_flag_testbed(rw, file, 0, __LINE__);

    // tb2d_io_create(rw, file, rootdir, tb2);

    fclose(file);

    return 0;
}