
#include "main_dataset.h"

#include "tb2d_io.h"

RTB2D main_dataset_generate(char dirpath[DIR_MAX], RTB2W tb2w ,const size_t n_try, MANY(FoldConfig) foldconfig_many) {
    RTB2D tb2d;

    tb2d = tb2d_create(tb2w, n_try, foldconfig_many);

    tb2d_run(tb2d);

    tb2d_io(IO_WRITE, dirpath, NULL, &tb2d);

    return tb2d;
}

RTB2D main_dataset_load(char dirpath[DIR_MAX], RTB2W tb2w) {
    RTB2D tb2d = NULL;

    if (tb2d_io(IO_READ, dirpath, tb2w, &tb2d)) {
        return NULL;
    }

    return tb2d;
}

int main_dataset_test(char dirpath[DIR_MAX], RTB2W tb2w, const size_t n_try, MANY(FoldConfig) foldconfig_many) {
    RTB2D tb2d_1, tb2d_2;

    #define _TEST(A) printf("%100s -> ", #A); puts((A) ? "success." : "failed."); if (!(A)) return -1;

    io_path_concat(dirpath, "test/", dirpath);

    tb2d_1 = main_dataset_generate(dirpath, tb2w, n_try, foldconfig_many);
    if (NULL == tb2d_1) {
        exit(-1);
    }
    tb2d_2 = main_dataset_load(dirpath, tb2w);
    if (NULL == tb2d_2) {
        exit(-1);
    }

    _TEST(tb2d_1->n.fold == tb2d_2->n.fold);
    _TEST(tb2d_1->n.try == tb2d_2->n.try);
    _TEST(tb2d_1->folds.number == tb2d_2->folds.number);
    for (size_t idxfold = 0; idxfold < tb2d_1->folds.number; idxfold++) {
        _TEST(tb2d_1->folds._[idxfold].k == tb2d_2->folds._[idxfold].k);
        _TEST(tb2d_1->folds._[idxfold].k_test == tb2d_2->folds._[idxfold].k_test);
    }

    _TEST(tb2d_1->folds.number == tb2d_2->folds.number);

    BY_FOR(*tb2d_1, try) {
        RDataset ds_1 = BY_GET(*tb2d_1, try).dataset;
        RDataset ds_2 = BY_GET(*tb2d_2, try).dataset;
        size_t wrongs;

        _TEST(ds_1->windows.all.number == ds_2->windows.all.number);
        _TEST(ds_1->windows.binary[0].number == ds_2->windows.binary[0].number);
        _TEST(ds_1->windows.binary[1].number == ds_2->windows.binary[1].number);
        _TEST(ds_1->windows.multi[0].number == ds_2->windows.multi[0].number);
        _TEST(ds_1->windows.multi[1].number == ds_2->windows.multi[1].number);
        _TEST(ds_1->windows.multi[2].number == ds_2->windows.multi[2].number);
    
        wrongs = 0;
        for (size_t i = 0; i < ds_1->windows.all.number; i++) {
            wrongs += !(ds_1->windows.all._[i]->index == ds_2->windows.all._[i]->index);
        }
        _TEST(wrongs == 0)

        wrongs = 0;
        for (size_t i = 0; i < ds_1->windows.binary[0].number; i++) {
            wrongs += !(ds_1->windows.binary[0]._[i]->index == ds_2->windows.binary[0]._[i]->index);
        }
        _TEST(wrongs == 0)

        wrongs = 0;
        for (size_t i = 0; i < ds_1->windows.binary[1].number; i++) {
            wrongs += !(ds_1->windows.binary[1]._[i]->index == ds_2->windows.binary[1]._[i]->index);
        }
        _TEST(wrongs == 0)

        DGAFOR(cl) {
            wrongs = 0;
            for (size_t i = 0; i < ds_1->windows.multi[cl].number; i++) {
                wrongs += !(ds_1->windows.multi[cl]._[i]->index == ds_2->windows.multi[cl]._[i]->index);
            }
            _TEST(wrongs == 0)
        }
    }

    tb2d_free(tb2d_1);
    tb2d_free(tb2d_2);

    return 0;
}