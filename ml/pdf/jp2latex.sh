#!/bin/sh

FILE=windows_time

jupyter nbconvert --to markdown --output-dir ./out --no-input "../$FILE.ipynb"

markdownlint-cli2 --fix ./out/windows_time.md

pandoc -f markdown+lists_without_preceding_blankline --write=latex "--output=./out/$FILE.tex" "$FILE.md"

cat head.tex "./out/$FILE.tex" tail.tex >> ./out/main.tex

# perl -i -pe 's/\\tightlist//' ./out/main.tex

pdflatex -output-directory ./out ./out/main.tex