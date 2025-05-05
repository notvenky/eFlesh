NAME=slides
SOURCES=slides.tex introduction.tex material_optimization.tex homogenization.tex patterns.tex pattern_optimization.tex
IMAGES=
all: $(NAME).pdf

force:
	pdflatex -shell-escape  "\documentclass{beamer}\input{$(NAME)}" && mv beamer.pdf $(NAME).pdf

view: $(NAME).pdf
	skim-load $(NAME).pdf

$(NAME).pdf: $(SOURCES) $(IMAGES)
	pdflatex -shell-escape  "\documentclass{beamer}\input{$(NAME)}" && mv beamer.pdf $(NAME).pdf

$(NAME)-handout.pdf: $(SOURCES)
	 pdflatex -shell-escape -job-name=slides-handout "\documentclass[handout]{beamer}\input{$(NAME)}" && mv beamer.pdf $(NAME)-handout.pdf
