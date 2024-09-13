#!/bin/sh

out="./out"
rm -fr "$out"
mkdir -p "$out"

for f in ../book/*.ipynb; do
    dn=$(dirname "$f")
    bn=$(basename "$f")
    bn=${bn%.*}

    
    TO_LATEX=True jupyter nbconvert --execute --to markdown --output-dir "$out" --no-input "$f"

    markdownlint-cli2 --fix "$out/$bn.md"

    (cd ./$out/ && pandoc -f markdown+lists_without_preceding_blankline --write=latex --output="$bn.tex" "$bn.md")
    # cat head.tex "./$out/$bn.tex" tail.tex >> "./$out/main.tex"
    cat "$out/$bn.tex" >> "$out/book.tex"
done

cp ../book/*.pdf "$out"

MAIN="$out/main.tex"
cat head.tex "$out/book.tex" tail.tex >> "$MAIN"

perl -i -pe 's/\\tightlist//' "$MAIN"
perl -i -pe 's/longtable/tabular/' "$MAIN"
perl -i -pe 's/\\endlastfoot//' "$MAIN"
perl -i -pe 's/\\endhead//' "$MAIN"

python transl.py "$MAIN"

latexindent "$MAIN" > "$MAIN.i"
rm "$MAIN" && mv "$MAIN.i" "$MAIN"

perl -i -pe 's/\n{2,}/\n/' "$MAIN"

(cd $out && pdflatex main.tex)
(cd $out && pdflatex main.tex)
(cd $out && pdflatex main.tex)

# jupyter nbconvert --execute --to markdown --output-dir ./out --no-input "../$FILE.ipynb"

# markdownlint-cli2 --fix "./out/$FILE.md"

# (cd ./out/ && pandoc -f markdown+lists_without_preceding_blankline --write=latex --output="$FILE.tex" "$FILE.md")

# cat head.tex "./out/$FILE.tex" tail.tex >> ./out/main.tex

# perl -i -pe 's/\\tightlist//' ./out/main.tex
# perl -i -pe 's/longtable/tabular/' ./out/main.tex
# perl -i -pe 's/\\endlastfoot//' ./out/main.tex
# perl -i -pe 's/\\endhead//' ./out/main.tex
# # perl -i -pe 's/\@(end|begin)\@(\w+)/\\$1{$2}/' ./out/main.tex
# # perl -i -pe 's/\@(label|caption)\@(.*?)\@/\\$1{$2}/' ./out/main.tex

# python transl.py

# cp ../*.pdf ./out/
# cp ../*.svg ./out/
# (cd ./out/ && latexindent main.tex > main.texi )
# (cd ./out/ && rm main.tex && mv main.texi main.tex )

# perl -i -pe 's/\n{2,}/\n/' ./out/main.tex

# (cd ./out/ && pdflatex ./main.tex)
# (cd ./out/ && pdflatex ./main.tex)
# (cd ./out/ && pdflatex ./main.tex)

# cp ./out/main.pdf .

# # mv ./out/latex/main.pdf .