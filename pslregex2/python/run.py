
from pslregex import PSLdict
import argparse

if __name__ == "__main__":

    parser = argparse.ArgumentParser(
                    prog='DNSMalwareDetection',
                    description='Malware detection')
    
    parser.add_argument("-r", "--redo", action="store_true", help="Do everything even if computed file exists.")
    parser.add_argument("-d", "--download", action="store_true", help="Download everything even if files to be downloaded exist.")
    parser.add_argument('outdir')

    args = parser.parse_args()

    print(f"[info]: saving all to {args.outdir}")
    print(f"[info]: <redo> is: {args.redo}")
    print(f"[info]: <download> is: {args.download}")

    psl = PSLdict(args.outdir)

    psl.init(args.redo, args.download)
    