
#ifndef __DATASET_H__
#define __DATASET_H__

#include "dn.h"

void dataset_fill(Windowing* windowing, Dataset* dt);

void dataset_traintest(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split);

void dataset_traintest_cm(PSets* psets, DatasetTrainTestPtr dt_tt, double *th, int32_t (*cm)[N_CLASSES][2]);

#endif