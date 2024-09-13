from pathlib import Path

import pandas as pd
import pslregex.common
import pslregex.etld
import pslregex.iana
import pslregex.tldlist



dir = Path("/home/princio/Repo/princio/malware_detection_preditc_file/pslregex2/python/examples")

etld = pslregex.etld.ETLD(dir)
tldlist = pslregex.tldlist.TLDLIST(dir)
iana = pslregex.iana.IANA(dir)
psl = pslregex.psl.PSL(dir)

tldlist.parse(etld)
iana.parse(etld)
psl.parse(etld)


with open("/tmp/psl.csv", "w") as fp:
    for k in etld.suffixes:
        suffix = etld.suffixes[k]
        print("%s," % suffix.suffix, file=fp, end="")
        if suffix.iana:
            print("%s," % suffix.iana.type.name, file=fp, end="")
            print("\"%s\"," % suffix.iana.tld_manager, file=fp, end="")
            pass
        else:
            [ print(",", file=fp, end="") for _ in pslregex.common.IANA_COLUMNS ]
            pass
        if suffix.tldlist:
            print("%s," % suffix.tldlist.type.name, file=fp, end="")
            print("%s," % suffix.tldlist.punycode, file=fp, end="")
            print("%s," % suffix.tldlist.language_code, file=fp, end="")
            print("%s," % suffix.tldlist.translation, file=fp, end="")
            print("%s," % suffix.tldlist.romanized, file=fp, end="")
            print("%s," % suffix.tldlist.rtl, file=fp, end="")
            print("\"%s\"," % suffix.tldlist.sponsor, file=fp, end="")
            pass
        else:
            [ print(",", file=fp, end="") for _ in pslregex.common.TLDLIST_COLUMNS ]
            pass
        if suffix.psl:
            print("%s," % suffix.psl.type.name, file=fp, end="")
            print("%s," % suffix.psl.punycode, file=fp, end="")
            print("%s" % suffix.psl.section, file=fp, end="")
            pass
        else:
            [ print(",", file=fp, end="") for _ in pslregex.common.PSL_COLUMNS ]
            pass
        print("", file=fp)
        pass
    pass