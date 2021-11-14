CC = gcc
CFLAGS = -Wall -Werror
LIBS = -lncurses

SRC = shime.c
EXE = shime
OBJ = $(SRC:.c=.o)
DESTDIR = /usr/local

.PHONY: all
all: $(EXE)

.PHONY: run
run: $(EXE)
	bin/shime

.PHONY: clean
clean:
	rm $(OBJ)
	rm shime

.PHONY: install
install: $(EXE)
	cp $(EXE) $(DESTDIR)/bin/$(EXE)

.PHONY: uninstall
uninstall: $(EXE)
	rm $(DESTDIR)/bin/$(EXE)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 
