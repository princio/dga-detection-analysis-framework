type OnlyFirsts = 'global' | 'pcap' | 'all';
type PacketType = 'qr' | 'q' | 'r' | 'nx';
enum NN {
    NONE = 1,
    TLD = 2,
    ICANN = 3,
    PRIVATE = 4
}

interface SlotConfig {
    sps: number;
    onlyfirsts: OnlyFirsts;
    packet_type: PacketType;
    nn: NN;
    th: number;
    wl_th?: number;
    wl_col?: 'dn' | 'bdn'
    pcap_id?: number;
    range: [number, number];
}