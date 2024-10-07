
from pathlib import Path
import ..psl_list
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

    _etld = psl_list.etld.ETLD(workdir)
    _tldlist = psl_list.tldlist.TLDLIST(workdir)
    _iana = psl_list.iana.IANA(workdir)
    _psl = psl_list.psl.PSL(workdir)

    _tldlist.parse(_etld)
    _iana.parse(_etld)
    _psl.parse(_etld)

    _etld.to_csv(output)