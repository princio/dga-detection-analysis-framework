
from typing import Union
from dataclasses import make_dataclass
import enum

class SuffixGroup(enum.Enum):
    tld = "tld"
    icann = "icann"
    private = "private"
    unknown = "unknown"
    pass

class ETLDType(enum.Enum):
    country_code = "cc"
    sponsored = "sp"
    infrastructure = "in"
    generic_restricted = "gr"
    generic = "ge"
    test = "te"
    pass

class Origin(enum.Enum):
    tldlist = "tldlist"
    iana = "iana"
    psl = "psl"
    pass



IANA_COLUMNS = [
        ("type", ETLDType, "Type"),
        ("tld_manager_code", str, "TLD Manager Code")
    ]

IANA_FIELDS = make_dataclass("IANA_FIELDS", IANA_COLUMNS, init=True)




TLDLIST_COLUMNS = [
        ("type", ETLDType, "Type"),
        ("punycode", str, "Punycode"),
        ("language_code", str, "Language Code"),
        ("translation", str, "Translation"),
        ("romanized", str, "Romanized"),
        ("rtl", str, "RTL"),
        ("sponsor", str, "Sponsor")
    ]

TLDLIST_FIELDS = make_dataclass("TLDLIST_FIELDS", TLDLIST_COLUMNS, init=True)


class PSLSection(enum.Enum):
    icann = "icann"
    icannnew = "icann-new"
    private = "private-domain"
    unknown = "unknown"

    @staticmethod
    def from_string(str):
        a = {}
        a[PSLSection.icann.value] = PSLSection.icann
        a[PSLSection.icannnew.value] = PSLSection.icannnew
        a[PSLSection.private.value] = PSLSection.private
        return a[str] if str in a else PSLSection.unknown
        
    pass


PSL_COLUMNS = [
        ("type", ETLDType, "Type"),
        ("punycode", str, "Punycode"),
        ("section", PSLSection, "Section")
    ]

PSL_FIELDS = make_dataclass("PSL_FIELDS", PSL_COLUMNS, init=True)
