all: main

main: main.tex report.cls
	kitty -d "$$PWD" nvim main.tex & disown
	zathura main.pdf & disown
	ls "main.tex" "report.cls" \
		| entr -s "pdflatex -interaction nonstopmode -shell-escape -file-line-error main.tex"

clean:
	rm -rf main.aux main.log main.toc main.out _minted-main
