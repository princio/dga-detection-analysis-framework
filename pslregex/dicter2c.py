import json
import pandas as pd
from pslregex import PSLdict

# psl = PSLdict('.')

# psl.init(True, True)

# psl.etld.iframe().to_csv('iframe.csv')

df = pd.read_csv('iframe.csv')

suffixes = df['4'].fillna('') + '.' + df['3'].fillna('') + '.' + df['2'].fillna('') + '.' + df['1'].fillna('') + '.' + df['0'].fillna('')

suffixes = suffixes.str.replace(r'\.{2,}', '.', regex=True)

suffixes = suffixes.str.replace(r'^\.', '', regex=True)

max = suffixes.apply(lambda x: len(bytes(x, 'utf-8'))).max()
argmax = suffixes.apply(lambda x: len(bytes(x, 'utf-8'))).argmax()


print(max)
print(suffixes.iloc[argmax])

for col in df.columns[1:]:
    print('{:20}'.format(col), df[col].drop_duplicates().fillna('').apply(lambda x: len(bytes(x, 'utf-8'))).max())

alphabet = set()

for item in suffixes:
    [alphabet.add(c) for c in item]

print(len(alphabet))
