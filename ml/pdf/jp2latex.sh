#!/bin/sh

rm -fr ./out/
mkdir -p ./out/latex

FILE="windows_time"

jupyter nbconvert --execute --to markdown --output-dir ./out --no-input "../$FILE.ipynb"

markdownlint-cli2 --fix "./out/$FILE.md"

(cd ./out/ && pandoc -f markdown+lists_without_preceding_blankline --write=latex --output="$FILE.tex" "$FILE.md")

cat head.tex "./out/$FILE.tex" tail.tex >> ./out/main.tex

perl -i -pe 's/\\tightlist//' ./out/main.tex
perl -i -pe 's/longtable/tabular/' ./out/main.tex
perl -i -pe 's/\\endlastfoot//' ./out/main.tex
perl -i -pe 's/\\endhead//' ./out/main.tex
# perl -i -pe 's/\@(end|begin)\@(\w+)/\\$1{$2}/' ./out/main.tex
# perl -i -pe 's/\@(label|caption)\@(.*?)\@/\\$1{$2}/' ./out/main.tex

python transl.py

cp ../*.pdf ./out/
cp ../*.svg ./out/
(cd ./out/ && latexindent main.tex > main.texi )
(cd ./out/ && rm main.tex && mv main.texi main.tex )

perl -i -pe 's/\n{2,}/\n/' ./out/main.tex

(cd ./out/ && pdflatex ./main.tex)
(cd ./out/ && pdflatex ./main.tex)
(cd ./out/ && pdflatex ./main.tex)

cp ./out/main.pdf .

# mv ./out/latex/main.pdf .