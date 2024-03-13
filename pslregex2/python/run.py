
from pathlib import Path
from tkinter import WORD
import pslregex
import argparse


if __name__ == "__main__":

    argp = argparse.ArgumentParser("suffixlist")

    argp.add_argument("-f", "--force", action="store_true", help="Forcing the update even if file exists")
    argp.add_argument("-w", "--workdir", default="/tmp/pslregex", help="The workdir used for download need files")
    argp.add_argument("output", help="The destination path for the list dataframe")

    args = argp.parse_args()


    output = Path(args.output)
    if not output.parent.exists():
        raise Exception("Directory where to save output list non exists")

    workdir = Path(args.workdir)
    workdir.mkdir(exist_ok=True, parents=True)


    _etld = pslregex.etld.ETLD(workdir)
    _tldlist = pslregex.tldlist.TLDLIST(workdir)
    _iana = pslregex.iana.IANA(workdir)
    _psl = pslregex.psl.PSL(workdir)

    _tldlist.parse(_etld)
    _iana.parse(_etld)
    _psl.parse(_etld)

    _etld.to_csv(output)