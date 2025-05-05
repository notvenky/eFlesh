NAME=periodic_homogenization
SOURCES=
IMAGES=
all: $(NAME).pdf

view: $(NAME).pdf
	skim-load $(NAME).pdf

$(NAME).pdf: $(NAME).tex $(NAME).bbl $(SOURCES) $(IMAGES)
	pdflatex -shell-escape $(NAME)

$(NAME).bbl: references.bib
	pdflatex -shell-escape $(NAME)
	bibtex $(NAME)
