NAME=shape_derivative
SOURCES=
IMAGES=
all: $(NAME).pdf

force:
	pdflatex -shell-escape $(NAME)

view: $(NAME).pdf
	skim-load $(NAME).pdf

$(NAME).pdf: $(NAME).tex ${NAME}.bbl $(SOURCES) $(IMAGES)
	pdflatex -shell-escape $(NAME)

$(NAME).bbl: references.bib
	pdflatex -shell-escape $(NAME)
	bibtex $(NAME)
