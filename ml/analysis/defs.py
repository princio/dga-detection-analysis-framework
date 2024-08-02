

from dataclasses import dataclass
import enum
from typing import Optional, Tuple, Union, List
import warnings

import pandasgui
from sqlalchemy import create_engine

def pgshow(dfs):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        if type(dfs) == list:
            pandasgui.show(*dfs)
        else:
            pandasgui.show(dfs)
    pass

class NN(enum.Enum):
    NONE = 1
    TLD = 2
    ICANN = 3
    PRIVATE = 4
    pass
    def __str__(self):
        return f"{self.value}"
    def __repr__(self):
        return self.__str__()
    pass

class PacketType(enum.StrEnum):
    QR = "QR"
    Q = "Q"
    R = "R"
    NX = "NX"
    OK = "OK"
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
class FetchConfig:
    sps: int = 1 * 60 * 60
    nn: NN = NN.NONE
    th: float = 0.5
    pcap_id: Optional[int] = None
    pcap_offset: float = 0
    pass

    def new_fromsource(self, pcap_id: Optional[int]):
        return FetchConfig(self.sps, self.nn, self.th, pcap_id)
    
    def new_offset(self, offset: float):
        return FetchConfig(self.sps, self.nn, self.th, self.pcap_id, offset)

    def __hash__(self):
        return hash((
            self.sps,
            self.nn.value,
            self.th,
            self.pcap_id,
            self.pcap_offset
        ))

    def __eq__(self, other):
        if not isinstance(other, FetchConfig):
            return NotImplemented
        return (
            self.sps == other.sps and
            self.nn == other.nn and
            self.th == other.th and
            self.pcap_id == other.pcap_id
            and self.pcap_offset == other.pcap_offset
        )
    
    def __str__(self):
        return (
            f"FetchConfig: [" +
            ", ".join([
                f"sps: {self.sps}",
                f"nn: {self.nn}",
                f"th: {self.th}",
                f"pcap_id: {self.pcap_id}"
                , f"pcap_offset: {self.pcap_offset}"
            ]) +
            "]"
        )

    def __repr__(self):
        return self.__str__()

