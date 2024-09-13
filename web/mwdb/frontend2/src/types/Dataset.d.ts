import { Pcap } from './Pcap';

export interface Dataset {
  id: string;
  name: string;
  pcaps: Pcap[];
}
