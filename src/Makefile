bin = gdice
# Append flags(possibly overriding existing ones) with make CFLAGS=<flags> ...
override CFLAGS := $(shell pkg-config --cflags gtk+-3.0 glib-2.0) -g -O2 -pedantic -Wall -std=c99 $(CFLAGS)
LDLIBS = $(shell pkg-config --libs gtk+-3.0 glib-2.0)
sources = $(wildcard *.c)
objects = $(sources:.c=.o)

.PHONY: all ctags clean

all: $(objects)
	$(CC) $(CFLAGS) $< -o ../$(bin) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDLIBS)

ctags:
	ctags -o ../tags -R --c++-kinds=+p --fields=+iaS --extra=+q \
	$(filter-out -p%, $(subst -I,, $(shell pkg-config --cflags gtk+-3.0 glib-2.0))) .

clean:
	@rm -f $(objects)
