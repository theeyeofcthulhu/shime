CC = gcc
LIBS = -lncurses $(shell pkg-config --libs sdl2)

SRC = shime.c
EXE = shime
OBJ = $(SRC:.c=.o)
DESTDIR = /usr/local

SOUND=Bell, Counter, A.wav

CFLAGS = -Wall -Wextra -std=c11 -pedantic -ggdb -DBUILD_SOUND_PATH="\"$(DESTDIR)/share/$(EXE)/$(SOUND)\""

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
	install -m755 "$(EXE)" "$(DESTDIR)/bin"
	mkdir -p "$(DESTDIR)/share/$(EXE)"
	install -m644 "$(SOUND)" "$(DESTDIR)/share/$(EXE)"

.PHONY: uninstall
uninstall:
	rm "$(DESTDIR)/bin/$(EXE)"
	rm "$(DESTDIR)/share/$(EXE)/$(SOUND)"
	rmdir "$(DESTDIR)/share/$(EXE)"

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) 
