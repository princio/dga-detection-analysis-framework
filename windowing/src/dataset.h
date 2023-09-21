
#ifndef __DATASET_H__
#define __DATASET_H__

#include "dn.h"

void dataset_fill(Windowing* windowing, Dataset* dt);

void dataset_traintest(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split);

void dataset_traintest_cm(int32_t wsize, PSets* psets, DatasetTrainTestPtr dt_tt, int32_t m_number, double th[m_number], ConfusionMatrix cm[m_number]);

#endif