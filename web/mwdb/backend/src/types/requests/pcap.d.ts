import { NN, PacketType, PacketGrouping } from '../common/common';

interface Plot {
  sps: int;
  onlyfirsts?: PacketGrouping;
  packet_type: PacketType;
  nn: NN;
  th: float;
  wl_th: int;
  wl_col: Optional;
}
