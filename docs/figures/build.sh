#!/bin/sh
# Rebuild the hero pipeline diagram: pipeline.tex -> pipeline.pdf -> pipeline.svg
#
# Requires: pdflatex (TeX Live, with tikz) and pdf2svg.
#   Debian/Ubuntu: apt install texlive-pictures texlive-latex-extra pdf2svg
#
# The committed artifact is pipeline.svg; the .pdf and TeX aux files are
# intermediates and are gitignored.

set -e
cd "$(dirname "$0")"

pdflatex -interaction=nonstopmode -halt-on-error pipeline.tex >/dev/null
pdf2svg pipeline.pdf pipeline.svg
rm -f pipeline.aux pipeline.log

echo "wrote $(pwd)/pipeline.svg"
