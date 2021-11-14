CC = gcc
CFLAGS = -Wall -Werror
LIBS = -lncurses

SRC = shime.c
EXE = $(ODIR)/shime
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))
ODIR = bin
IDIR = /usr/local/bin

.PHONY: build
build: $(EXE)

.PHONY: run
run: $(EXE)
	bin/shime

.PHONY: clean
clean:
	@rm -rf $(ODIR)
	@echo "removing local bin directory"
	@sudo rm $(IDIR)/shime
	@echo "removing shime bin from $(IDIR)"

$(ODIR):
	mkdir -p $@

$(ODIR)/%.o: %.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

install: $(EXE)
	@echo "copying shime bin to $(IDIR)"
	@sudo cp $(ODIR)/shime $(IDIR)/shime
