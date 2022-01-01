CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11 -pedantic -ggdb
LIBS = -lncurses

SRC = shime.c
EXE = shime
OBJ = $(SRC:.c=.o)
DESTDIR = /usr/local

.PHONY: all
all: $(EXE)

.PHONY: run
run: $(EXE)
	./$(EXE)

.PHONY: clean
clean:
	rm $(OBJ)
	rm $(EXE)

.PHONY: install
install: $(EXE)
	cp $(EXE) $(DESTDIR)/bin/$(EXE)

.PHONY: uninstall
uninstall: $(EXE)
	rm $(DESTDIR)/bin/$(EXE)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) 
