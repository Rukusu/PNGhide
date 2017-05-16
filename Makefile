CC = clang
IDIR = include
ODIR = bin
SDIR = src
CFLAGS= -I$(IDIR) -Wall -lm
LIBS = -lpng

all: encoder decoder tester

encoder: $(SDIR)/PNGhideEncoder.c
	$(CC) $(LIBS) -o $(ODIR)/PNGhideEncoder $(SDIR)/PNGhideEncoder.c $(CFLAGS)

decoder: $(SDIR)/PNGhideDecoder.c
	$(CC) $(LIBS) -o $(ODIR)/PNGhideDecoder $(SDIR)/PNGhideDecoder.c $(CFLAGS)

tester: $(SDIR)/PNGhideTestImages.c
	$(CC) $(LIBS) -o $(ODIR)/PNGhideTestImages $(SDIR)/PNGhideTestImages.c $(CFLAGS)

