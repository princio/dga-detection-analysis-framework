#include "fold.h"

#include "dataset.h"
#include "gatherer.h"
#include "io.h"
#include "testbed2.h"
#include "window0s.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FALSERATIO(tf) (((double) (tf).falses) / ((tf).falses + (tf).trues))
#define TRUERATIO(tf) (((double) (tf).trues) / ((tf).falses + (tf).trues))

#define MIN(A, B) (A.min) = ((B) <= (A.min) ? (B) : (A.min))
#define MAX(A, B) (A.max) = ((B) >= (A.max) ? (B) : (A.max))

RGatherer folds_gatherer = NULL;

// void fold_free(void* item) {
//     RFold fold = (RFold) item;
//     for (size_t try = 0; try < fold->tries.number; try++) {
//         for (size_t wsize = 0; wsize < fold->tries._[try].bywsize.number; wsize++) {
//             FREEMANY(fold->tries._[try].bywsize._[wsize].splits.splits);
//         }
//         FREEMANY(fold->tries._[try].bywsize);
//     }
//     FREEMANY(fold->tries);
// }

// RFold fold_create(RTestBed2 tb2, FoldConfig config) {
//     if (config.k <= 1 || (config.k - 1) < config.k_test) {
//         fprintf(stderr, "Error within kfolds (%ld) and test_folds (%ld).", config.k, config.k_test);
//         exit(1);
//     }

//     RFold fold;

//     if (folds_gatherer == NULL) {
//         gatherer_alloc(&folds_gatherer, "folds", fold_free, 50, sizeof(__Fold), 5);
//     }

//     fold = gatherer_alloc_item(folds_gatherer);

//     fold->tb2 = tb2;
//     fold->config = config;

//     INITMANY(fold->tries, config.tries, FoldTry);
//     for (size_t try = 0; try < config.tries; try++) {
//         INITMANY(fold->tries._[try].bywsize, tb2->wsizes.number, FoldTryWSize);
//     }

//     return fold;
// }

// void fold_add(RTestBed2 tb2, FoldConfig config) {
//     RFold fold;

//     fold = fold_create(tb2, config);

//     gatherer_many_add((__MANY*) &tb2->folds, sizeof(RFold), 5, fold);

//     for (size_t try = 0; try < config.tries; try++) {
//         for (size_t idxwsize = 0; idxwsize < TB2_WSIZES_N(tb2); idxwsize++) {
//             fold->tries._[try].bywsize._[idxwsize].splits = dataset0_splits(tb2->datasets.bywsize._[idxwsize], config.k, config.k_test);
//         }
//     }
// }

// void fold_io(IOReadWrite rw, FILE* file, RTestBed2 tb2, RFold* fold) {
//     FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

//     FoldConfig kconfig;

//     if (rw == IO_WRITE) {
//         kconfig = (*fold)->config;
//     }

//     FRW(kconfig.tries);
//     FRW(kconfig.k);
//     FRW(kconfig.k_test);

//     if (rw == IO_READ) {
//         (*fold) = fold_create(tb2, kconfig);
//     }

//     for (size_t t = 0; t < kconfig.tries; t++) {
//         for (size_t w = 0; w < (*fold)->tb2->wsizes.number; w++) {
//             DatasetSplits* splits;

//             splits = &(*fold)->tries._[t].bywsize._[w].splits;
        
//             FRW(splits->isok);
            
//             if (!splits->isok) {
//                 printf("Warning: skipping dataset splits for wsize=%ld.\n",  (*fold)->tb2->wsizes._[w].value);
//             } else {
//                 if (rw == IO_READ) {
//                     INITMANY(splits->splits, (*fold)->config.k, DatasetSplit0);
//                 }
//                 for (size_t k = 0; k < (*fold)->config.k; k++) {
//                     testbed2_io_dataset(rw, file, (*fold)->tb2, (*fold)->tb2->wsizes._[w], &splits->splits._[k].train);
//                     testbed2_io_dataset(rw, file, (*fold)->tb2, (*fold)->tb2->wsizes._[w], &splits->splits._[k].test);
//                 }
//             }
//         }
//     }
// }

// void fold_md(FILE* file, const RFold fold) {
//     #define FP(...) fprintf(file, __VA_ARGS__);
//     #define FPNL(N, ...) fprintf(file, __VA_ARGS__); for (size_t i = 0; i < N; i++) fprintf(file, "\n");

//     FPNL(1, "- N=%ld", fold->config.tries);
//     FPNL(1, "- K=%ld", fold->config.k);
//     FPNL(2, "- K for testing: %ld", fold->config.k_test);

//     const int len_header = 15;
//     {
//         const char* headers[] = {
//             "wsize",
//             "folded",
//             "0 train avg",
//             "0 test avg",
//             "1 train avg",
//             "1 test avg",
//             "2 train avg",
//             "2 test avg",
//         };
//         const size_t n_headers = sizeof (headers) / sizeof (const char *);
//         for (size_t i = 0; i < n_headers; i++) {
//             FP("|%*s", len_header, headers[i]);
//         }
//         FPNL(1, "|");
//         for (size_t i = 0; i < n_headers; i++) {
//             FP("|");
//             for (int t = 0; t < len_header; t++) {
//                 FP("-");
//             }
//         }
//         FPNL(1, "|");
//     }

//     for (size_t t = 0; t < fold->tries.number; t++) {
//         for (size_t w = 0; w < fold->tries._[t].bywsize.number; w++) {
//             DatasetSplits* splits = &fold->tries._[t].bywsize._[w].splits;
//             FP("|%*ld", len_header, fold->tb2->wsizes._[w].value);
//             FP("|%*d", len_header, splits->isok);
//             DGAFOR(cl) {
//                 float train_size = 0;
//                 float test_size = 0;
//                 float train_logit_avg = 0;
//                 float test_logit_avg = 0;
//                 for (size_t k = 0; k < splits->splits.number; k++) {
//                     DatasetSplit0* split = &splits->splits._[k];
//                     train_size += split->train->windows.multi[cl].number;
//                     test_size += split->test->windows.multi[cl].number;
//                 }
//                 FP("|%*.2f", len_header, train_size / splits->splits.number);
//                 FP("|%*.2f", len_header, test_size / splits->splits.number);
//             }
//         }
//     }

//     #undef FP
//     #undef FPNL
// }