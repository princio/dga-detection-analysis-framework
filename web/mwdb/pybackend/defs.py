

from dataclasses import dataclass
import enum
from typing import Optional, Tuple, Union, List

from sqlalchemy import create_engine
from flask_cors import CORS


class NN(enum.Enum):
    NONE = 1
    TLD = 2
    ICANN = 3
    PRIVATE = 4
    pass

class PacketType(enum.Enum):
    QR = "QR"
    Q = "Q"
    R = "R"
    NX = "NX"
    def __str__(self):
        return self.value
    def __repr__(self):
        return self.__str__()
    pass

class OnlyFirsts(enum.Enum):
    GLOBAL = "global"
    PCAP = "pcap"
    ALL = "all"
    def __str__(self):
        return self.value
    def __repr__(self):
        return self.__str__()
    pass

class WLFIELD(enum.StrEnum):
    DN="DN"
    BDN="BDN"
    def __str__(self):
        return self.value
    def __repr__(self):
        return self.__str__()

class MWTYPE(enum.StrEnum):
    NIC="no-malware"
    NONDGA="non-dga"
    DGA="dga"
    def __str__(self):
        return self.value
    def __repr__(self):
        return self.__str__()


@dataclass
class Database:
    uri = f"postgresql+psycopg2://postgres:@localhost:5432/dns_mac"

    def __init__(self):
        self.engine = create_engine(self.uri)
        # self.conn = self.engine.connect()
        pass


@dataclass(frozen=True)
class SlotConfig:
    sps: int = 1 * 60 * 60
    onlyfirsts: OnlyFirsts = OnlyFirsts.GLOBAL
    packet_type: PacketType = PacketType.NX
    nn: NN = NN.NONE
    th: float = 0.999
    wl_th: int = 10000
    wl_col: WLFIELD = WLFIELD.DN
    pcap_id: Optional[int] = None
    range: Tuple[int,int] = (0,10)

    def __hash__(self):
        return hash((self.sps,self.onlyfirsts,self.packet_type,self.nn,self.th,self.wl_th,self.wl_col,self.pcap_id,self.range))

    def __eq__(self, other):
        if not isinstance(other, SlotConfig):
            return NotImplemented
        return (
            self.sps == other.sps and
            self.onlyfirsts == other.onlyfirsts and
            self.packet_type == other.packet_type and
            self.nn == other.nn and
            self.th == other.th and
            self.wl_th == other.wl_th and
            self.wl_col == other.wl_col and
            self.pcap_id == other.pcap_id and
            self.range == other.range
        )
    
    def __str__(self):
        return (
            f"SlotConfig: ["
            f"sps: {self.sps}, "
            f"onlyfirsts: {self.onlyfirsts}, "
            f"packet_type: {self.packet_type}, "
            f"nn: {self.nn}, "
            f"th: {self.th}, "
            f"wl_th: {self.wl_th}, "
            f"wl_col: {self.wl_col}"
            f"pcap_id: {self.pcap_id}"
            f"range: {self.range}"
            "]"
        )

    def __repr__(self):
        return self.__str__()
