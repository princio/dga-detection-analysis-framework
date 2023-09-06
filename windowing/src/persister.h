#ifndef __PERSISTER_H__
#define __PERSISTER_H__

#include "dn.h"

int write_Pi_file(char path[], Pi* pis, const int N_PIS);

int read_Pi_file(char path[], Pi** pis, int* N_PIS);


int write_PCAP_file(char path[], PCAP* pcaps, const int N_PCAP);

int read_PCAP_file(char path[], PCAP** pcaps, int* N_PCAP);

void persister_test();

#endif