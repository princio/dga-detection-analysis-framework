
#ifndef __TRAINER_DETECTIONS_H__
#define __TRAINER_DETECTIONS_H__

#include "common.h"

#include "tb2d.h"

void trainer_detections_run(DatasetSplit split, MANY(ThsDataset) ths, size_t n_configs, MANY(Detection) detections[n_configs]);

#endif