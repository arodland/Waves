PROG   = intwaves-gl

OPTIMIZE ?= -O3 -march=native
DEBUG    ?=
WARN     ?=

CC     = gcc
LIBS   = -lSDL -lGLU -lGL -std=c99
CFLAGS = $(OPTIMIZE) $(WARN) $(DEBUG)

$(PROG): $(PROG).c colors.i Makefile
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

antiburnin: antiburnin.c Makefile
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
