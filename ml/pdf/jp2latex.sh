#!/bin/sh

rm -fr ./out/
mkdir -p ./out/latex

FILE="windows_time"

jupyter nbconvert --to markdown --output-dir ./out --no-input "../$FILE.ipynb"

markdownlint-cli2 --fix "./out/$FILE.md"

pandoc -f markdown+lists_without_preceding_blankline --write=latex "--output=./out/$FILE.tex" "./out/$FILE.md"

cat head.tex "./out/$FILE.tex" tail.tex >> ./out/main.tex

perl -i -pe 's/\\tightlist//' ./out/main.tex
perl -i -pe 's/longtable/tabular/' ./out/main.tex
perl -i -pe 's/\\endlastfoot//' ./out/main.tex
# perl -i -pe 's/\@(end|begin)\@(\w+)/\\$1{$2}/' ./out/main.tex
# perl -i -pe 's/\@(label|caption)\@(.*?)\@/\\$1{$2}/' ./out/main.tex

python transl.py

# pdflatex -output-directory ./out/latex/ ./out/main.tex

# mv ./out/latex/main.pdf .