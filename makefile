CC = gcc
CFLAGS = -I.
LIBS = -lncurses

SRC = shime.c util.c
DEPS = shime.h util.h
EXE = $(ODIR)/shime
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))
ODIR = bin
IDIR = /usr/local/bin

build: $(EXE)

clean:
	@rm -rf $(ODIR)
	@echo "removing local bin directory"
	@sudo rm $(IDIR)/shime
	@echo "removing shime bin from $(IDIR)"

$(ODIR):
	mkdir -p $@

$(ODIR)/%.o: %.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(LIBS) $(CFLAGS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

install: $(EXE)
	@echo "copying shime bin to $(IDIR)"
	@sudo cp $(ODIR)/shime $(IDIR)/shime
