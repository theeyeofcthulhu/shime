LIBS=-lncurses

build:	util.c shime.c
	gcc -o shime util.c shime.c $(LIBS)
