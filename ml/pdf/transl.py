# coding=utf8
# the above tag defines encoding for this document and is for Python 2.x compatibility

import re
import sys

regexs = []

# regexs.append((r"@@(begin|end)@@(.*?)@@", r"\\\1{\2}"))
# regexs.append((r"@@(label)@@(.*?)@@", r"\\\1{\2}"))
# regexs.append((r"@@(caption)@@(.*?)@@", r"\\\1{\2}"))
regexs.append((r"@@", r"\\"))
regexs.append((r"@:", r"{"))
regexs.append((r":@", r"}"))
regexs.append((r"@q", r"["))
regexs.append((r"q@", r"]"))

with open(sys.argv[1], "r") as f:
    print(f.name)
    lines = f.readlines()

subst = r"\\\1(\2)"

for regex in regexs:
    for line in lines:
        result = re.sub(regex[0], regex[1], line, 0, re.MULTILINE)
        # if result != line:
        #     print(f"{regex[0]}|-\t\t-|{line}|-\t\t-|{result}")

# print(len(lines))

text = "\n".join(lines)
for regex in regexs:
    text = re.sub(regex[0], regex[1], text, 0, re.MULTILINE)
text = re.sub(r"\n{2,}", r"\n", text, 0, re.MULTILINE)

# if result:
with open(sys.argv[1], "w") as f:
    f.write(text)

# Note: for Python 2.7 compatibility, use ur"" to prefix the regex and u"" to prefix the test string and substitution.