NAME=approach
all: view

view: $(NAME).pdf
	skim-load $(NAME).pdf

$(NAME).pdf: $(NAME).tex $(NAME).bbl examples.tex implementation.tex
	pdflatex -shell-escape $(NAME)

$(NAME).bbl: references.bib
	pdflatex -shell-escape $(NAME)
	bibtex $(NAME)
