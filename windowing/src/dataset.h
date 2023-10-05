
#ifndef __DATASET_H__
#define __DATASET_H__

#include "dn.h"

void dataset_fill(Windowing* windowing, Dataset* dt);

void dataset_traintestsplit(Dataset* dt, DatasetTrainTestPtr dt_tt, double percentage_split);

void dataset_traintestsplit_cm(int32_t wsize, PSets* psets, DatasetTrainTestPtr dt_tt);

#endif