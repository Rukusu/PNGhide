CC = clang
IDIR = include
ODIR = bin
SDIR = src
CFLAGS= -I$(IDIR) -Wall
LIBS = -lpng

all: encoder decoder

encoder: $(SDIR)/PNGhideEncoder.c
	$(CC) $(LIBS) -o $(ODIR)/PNGhideEncoder $(SDIR)/PNGhideEncoder.c $(CFLAGS)

decoder: $(SDIR)/PNGhideDecoder.c
	$(CC) $(LIBS) -o $(ODIR)/PNGhideDecoder $(SDIR)/PNGhideDecoder.c $(CFLAGS)
