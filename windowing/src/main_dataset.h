
#ifndef __MAIN_DATASET_H__
#define __MAIN_DATASET_H__

#include "common.h"

#include "tb2d.h"

RTB2D main_dataset_generate(char dirname[DIR_MAX], RTB2W tb2w, const size_t n_try, MANY(FoldConfig) foldconfig_many);
RTB2D main_dataset_load(char dirpath[DIR_MAX], RTB2W tb2w);
int main_dataset_test(char dirpath[DIR_MAX], RTB2W tb2w, const size_t n_try, MANY(FoldConfig) foldconfig_many);

#endif