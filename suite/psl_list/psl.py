
from typing import List, Union
import pandas as pd
import requests
import re

from .common import PSL_FIELDS, ETLDType,  Origin, PSLSection
from .etld import ETLD

class PSL:
    def __init__(self, dir) -> None:
        self.dir = dir
        self.file = dir.joinpath("public_suffix_list.dat")
        pass

    def download(self, force) -> Union[List[str],None]:
        if force or not self.file.exists():
            lines = requests.get(
                'https://publicsuffix.org/list/public_suffix_list.dat',
                verify=False,
                allow_redirects=True,
                timeout=5
            ).text
            with open(self.file, 'w') as f:
                f.writelines(lines)
                pass
            pass

        with open(self.file, 'r') as f:
            lines = [ l.replace('\n', '') for l in f.readlines() ]
            pass

        return lines
        
    def parse(self, etld: ETLD, force=False):
        lines = self.download(force)

        if lines is None:
            return

        sections_delimiters = [
            '// ===BEGIN ICANN DOMAINS===', '// newGTLDs', '// ===BEGIN PRIVATE DOMAINS==='
        ]

        sections_names = [ 'icann', 'icann-new', 'private-domain' ]

        regex_punycode = r'^\/\/ (xn--.*?) .*$'
        regex_comment = r'^\/\/ (?!Submitted)(.*?)(?: : )(.*?)$'

        line_start = 1 + lines.index(sections_delimiters[0])

        # Take attention to punycodes parsing: a new punycode should be used only for the next PSL
        
        sd = 0
        punycode = None
        values = []
        last_tld = ''
        punycode_found = False
        for i in range(line_start, len(lines)):
            line = lines[i]
            if len(line) == 0: continue
            if sd+1 < len(sections_delimiters) and line.find(sections_delimiters[sd+1]) == 0:
                sd += 1
            if line.find('//') == 0:
                punycode_match = re.match(regex_punycode, line)
                if punycode_match is not None:
                    punycode_found = True
                    punycode = punycode_match[1]
                else:
                    first_comment_match = re.match(regex_comment, line)
                    if first_comment_match is not None:
                        manager = first_comment_match[1]
                continue

            suffix = line
            tld = suffix[suffix.rfind('.')+1:]

            # if the tld (not the suffix) changed, the punycode changed too
            if last_tld != tld and not punycode_found:
                punycode = None
            punycode_found = False

            values.append([ suffix, tld, punycode, sections_names[sd] ]) #, manager

            last_tld = tld
            
            pass

        df = pd.DataFrame(values, columns=[ 'suffix', 'tld', 'punycode', 'section'])

        for _, row in df.iterrows():
            suffix = row["suffix"]

            fields = PSL_FIELDS(ETLDType.generic, row["punycode"], PSLSection.from_string(row["section"]))

            etld.add(suffix, Origin.psl, fields)
            pass


        return df.reset_index()