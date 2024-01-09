
#ifndef __MAIN_TRAINING_H__
#define __MAIN_TRAINING_H__

#include "common.h"

#include "trainer.h"

void main_training(char dirname[DIR_MAX], RTB2D tb2d);
RTrainer main_training_load(char dirname[DIR_MAX]);

#endif