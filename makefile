LIBS = -lncurses
DEPS = util.h
CC = gcc
CFLAGS = -I.
ODIR = bin
SRC = shime.c util.c
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))
EXE = $(ODIR)/shime

all: $(EXE)

clean:
	rm -rf $(ODIR)

$(ODIR):
	mkdir -p $@

$(ODIR)/%.o: %.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)
