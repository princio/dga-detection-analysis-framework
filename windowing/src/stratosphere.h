
#ifndef __STRATOSPHERE_H__
#define __STRATOSPHERE_H__

#include "dataset.h"
#include "experiment.h"

typedef struct StratosphereInfo { 
    int32_t id;
} StratosphereInfo;

/**
 * @brief It execute the windowing for the pcap and the number of windowing size chosen, in order to execute the pcap retrieve from
 * the database only one time for all the windowing sizes.
 * 
 * @param conn 
 * @param pcap 
 * @param windowings An array of PCAPWindowingPtr struct where each element represent the windowing of the PCAP for a defined window size.
 * @param N_WINDOWING DGAe number of window size we chose.
 */
void stratosphere_run();

#endif