# coding=utf8
# the above tag defines encoding for this document and is for Python 2.x compatibility

import re

regex = r"@(label)@(.*?)@"

with open("./out/main.tex", "r") as f:
    print(f.name)
    test_str = "\n".join(f.readlines())
    lines = f.readlines()

subst = r"\\\1(\2)"

# You can manually specify the number of replacements by changing the 4th argument
for line in lines:
    result = re.sub(regex, subst, test_str, 0, re.MULTILINE)
    if result:
        print(result)

print(len(lines))

result = re.sub(regex, subst, test_str, 0, re.MULTILINE)
if result:
    with open("./out/main.tex", "w") as f:
        f.write(result)

# Note: for Python 2.7 compatibility, use ur"" to prefix the regex and u"" to prefix the test string and substitution.