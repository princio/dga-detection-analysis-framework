
#ifndef __STRATOSPHERE_H__
#define __STRATOSPHERE_H__

#include "dn.h"

void dataset_fill(Windowing* windowing, Dataset dt[]);

void dataset_traintest(DatasetPtr dt, DatasetTrainTestPtr dt_tt, double percentage_split);

void dataset_traintest_cm(PSets* psets, DatasetTrainTestPtr dt_tt, double *th, int (*cm)[N_CLASSES][2]);

#endif