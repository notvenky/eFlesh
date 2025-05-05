NAME=objective
SOURCES=
IMAGES=
all: $(NAME).pdf

force:
	pdflatex -shell-escape $(NAME)

view: $(NAME).pdf
	skim-load $(NAME).pdf

$(NAME).pdf: $(NAME).tex $(SOURCES) $(IMAGES)
	pdflatex -shell-escape $(NAME)

$(NAME).bbl: references.bib
	pdflatex -shell-escape $(NAME)
	bibtex $(NAME)
