bin = gdice
CFLAGS = $(shell pkg-config --cflags gtk+-3.0 glib-2.0) -g -O2 -pedantic -Wall -std=c99
# fileno()
CFLAGS += -D_XOPEN_SOURCE
# vasprintf()
CFLAGS += -D_GNU_SOURCE
LDLIBS = $(shell pkg-config --libs gtk+-3.0 glib-2.0)
sources = $(wildcard *.c)
objects = $(sources:.c=.o)

.PHONY: all ctags clean

all: $(objects) diceexpr
	$(CC) $(CFLAGS) str.o de.tab.c lex.yy.c $< -o ../$(bin) $(LDLIBS)

diceexpr: str.o de.tab.c lex.yy.c

de.tab.c: de.y str.o
	bison -d $<

lex.yy.c: de.l
	flex $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDLIBS)

ctags:
	ctags -o ../tags -R --c++-kinds=+p --fields=+iaS --extra=+q \
	$(filter-out -p%, $(subst -I,, $(shell pkg-config --cflags gtk+-3.0 glib-2.0))) .

clean:
	-rm -f *.o de.tab.c de.tab.h lex.yy.c
