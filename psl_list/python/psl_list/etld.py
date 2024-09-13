
from dataclasses import dataclass
import enum
from pathlib import Path
from typing import Any, Dict, List, Union
import pandas as pd
import requests
import re, os
from io import StringIO

import psl_list.common

### Coding:
###
### A code is composed by: S00000TTE1
### Where:
### - S: the section (icann, icann-new, private-domain)
### - 00000: five digits which indicates the index in the etld frame
### - TT: two letters which indicates the type
### - E: if is an exception suffix
### - 1: the number of labels


@dataclass
class Suffix:
    suffix: str
    nlabels: int

    tldlist: Union[None, psl_list.common.TLDLIST_FIELDS]
    iana: Union[None, psl_list.common.IANA_FIELDS]
    psl: Union[None, psl_list.common.PSL_FIELDS]

    group: psl_list.common.SuffixGroup = psl_list.common.SuffixGroup.unknown
    from_tldlist: int = 0
    from_iana: int = 0
    from_psl: int = 0

    pass

class ETLD:
    def __init__(self, dir) -> None:
        self.files = {}
        self.dir = dir
        self.frame = None
        self.suffixes: Dict[str, Suffix] = {}
        pass

    def add(self, suffix: str, origin: psl_list.common.Origin, fields: Union[psl_list.common.TLDLIST_FIELDS, psl_list.common.IANA_FIELDS, psl_list.common.PSL_FIELDS]):
        suffix_found = self.suffixes.get(suffix)
        if suffix_found is None:
            suffix_found = Suffix(suffix, (suffix.count(".") + 1), None, None, None)
            pass

        if origin is psl_list.common.Origin.tldlist:
            suffix_found.from_tldlist += 1
            if not suffix_found.tldlist is None:
                print("Warning: tldlist is not none")
                pass
            suffix_found.tldlist = fields
            pass
        elif origin is psl_list.common.Origin.iana:
            suffix_found.from_iana += 1
            if not suffix_found.iana is None:
                print("Warning: iana is not none")
                pass
            suffix_found.iana = fields
            pass
        elif origin is psl_list.common.Origin.psl:
            suffix_found.from_psl += 1
            if not suffix_found.psl is None:
                print("Warning: psl is not none")
                pass
            suffix_found.psl = fields
            pass

        self.suffixes[suffix] = suffix_found
        pass

    def determine_suffix_group(self, suffix: Suffix):
        if suffix.nlabels == 1:
            return psl_list.common.SuffixGroup.tld
        if suffix.psl:
            if suffix.psl.section is psl_list.common.PSLSection.private:
                return psl_list.common.SuffixGroup.private
            else:
                return psl_list.common.SuffixGroup.icann
        return psl_list.common.SuffixGroup.unknown

    def tolist(self):
        suffixes = []
        for k in self.suffixes:
            suffix = self.suffixes[k]
            suffix_list = []
            suffix_list += [ suffix.suffix, self.determine_suffix_group(suffix).name, suffix.nlabels, suffix.from_psl, suffix.from_tldlist, suffix.from_iana ]
            if suffix.psl:
                suffix_list += [ suffix.psl.type.name ]
                suffix_list += [ suffix.psl.punycode ]
                suffix_list += [ suffix.psl.section.name ]
                pass
            else:
                [ suffix_list.append(None) for _ in psl_list.common.PSL_COLUMNS ]
                pass
            if suffix.tldlist:
                suffix_list += [ suffix.tldlist.type.name ]
                suffix_list += [ suffix.tldlist.punycode ]
                suffix_list += [ suffix.tldlist.language_code ]
                suffix_list += [ suffix.tldlist.translation ]
                suffix_list += [ suffix.tldlist.romanized ]
                suffix_list += [ suffix.tldlist.rtl ]
                suffix_list += [ suffix.tldlist.sponsor ]
                pass
            else:
                [ suffix_list.append(None) for _ in psl_list.common.TLDLIST_COLUMNS ]
                pass
            if suffix.iana:
                suffix_list += [ suffix.iana.type.name ]
                suffix_list += [ suffix.iana.tld_manager ]
                pass
            else:
                [ suffix_list.append(None) for _ in psl_list.common.IANA_COLUMNS ]
                pass
            suffixes += [suffix_list]
            pass
        return suffixes

    def to_csv(self, output: Path):
        suffixes = self.tolist()

        columns = [ ("", "suffix"), ("", "group"), ("", "nlabels"), ("", "psl"), ("", "tldlist"), ("", "iana") ]
        for n, columns_def in [ ( "psl", psl_list.common.PSL_COLUMNS), ( "tldlist", psl_list.common.TLDLIST_COLUMNS), ( "iana", psl_list.common.IANA_COLUMNS) ]:
            for column_def in columns_def:
                columns += [ ( n, column_def[0]) ]
                pass
            pass

        df = pd.DataFrame(suffixes, columns=pd.MultiIndex.from_tuples(columns))
        df = df.sort_values(by=("","suffix"))
        df = df.reset_index(drop=True)

        ascii = df[('', 'suffix')].apply(lambda s: 1 if s.isascii() else 0)

        df.insert(2, ('', 'ascii'), ascii)

        df.to_csv(output)
        
        return df
    pass