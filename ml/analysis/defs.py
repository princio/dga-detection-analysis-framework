

from dataclasses import dataclass
import enum
from typing import Optional, Tuple, Union, List

from sqlalchemy import create_engine


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
class FetchConfig:
    sps: int = 1 * 60 * 60
    nn: NN = NN.NONE
    th: float = 0.5
    table_from: Optional[int] = None
    pcap_offset: int = 0

    def __hash__(self):
        return hash((
            self.sps,
            self.nn,
            self.th,
            self.table_from,
            self.pcap_offset
        ))

    def __eq__(self, other):
        if not isinstance(other, FetchConfig):
            return NotImplemented
        return (
            self.sps == other.sps and
            self.nn == other.nn and
            self.th == other.th and
            self.table_from == other.table_from and
            self.pcap_offset == other.pcap_offset
        )
    
    def __str__(self):
        return (
            f"SlotConfig: [" +
            ", ".join([
                f"sps: {self.sps}",
                f"nn: {self.nn}",
                f"th: {self.th}",
                f"table_from: {self.table_from}",
                f"pcap_offset: {self.pcap_offset}"
            ]) +
            "]"
        )

    def __repr__(self):
        return self.__str__()
