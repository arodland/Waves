PROG   = intwaves-gl

OPTIMIZE ?= -O3 -march=nocona
DEBUG    ?=
WARN     ?=

CC     = gcc
LIBS   = -lSDL -lGLU -lGL -std=c99
CFLAGS = $(OPTIMIZE) $(WARN) $(DEBUG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
