CC = gcc
CFLAGS = -I.
LIBS = -lncurses

SRC = shime.c util.c
DEPS = shime.h util.h
EXE = $(ODIR)/shime
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))
ODIR = bin

build: $(EXE)

clean:
	rm -rf $(ODIR)
	if [ -e /usr/bin/shime ]; then sudo rm /usr/bin/shime; fi

$(ODIR):
	mkdir -p $@

$(ODIR)/%.o: %.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(LIBS) $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

install: $(EXE)
	if [ ! -e /usr/bin/shime ]; then  sudo cp $(ODIR)/shime /usr/bin/shime; fi
