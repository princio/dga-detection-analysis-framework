
#ifndef __STRATOSPHERE_H__
#define __STRATOSPHERE_H__

#include "dn.h"

void stratosphere_add_captures(WindowingPtr windowing);

/**
 * @brief It execute the windowing for the pcap and the number of windowing size chosen, in order to execute the pcap retrieve from
 * the database only one time for all the windowing sizes.
 * 
 * @param conn 
 * @param pcap 
 * @param windowings An array of PCAPWindowingPtr struct where each element represent the windowing of the PCAP for a defined window size.
 * @param N_WINDOWING The number of window size we chose.
 */
void stratosphere_procedure(WindowingPtr windowing, int32_t capture_index);

void stratoshere_disconnect();

#endif