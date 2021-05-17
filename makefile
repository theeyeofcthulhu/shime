CC = gcc
CFLAGS = -I.
LIBS = -lncurses

SRC = shime.c
DEPS = shime.h
EXE = $(ODIR)/shime
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))
ODIR = bin
IDIR = /usr/local/bin

run: $(EXE)
	bin/shime

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
