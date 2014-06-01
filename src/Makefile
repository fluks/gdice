bin = gdice
CFLAGS = $(shell pkg-config --cflags gtk+-3.0 glib-2.0 gstreamer-1.0) \
	-g -pedantic -Wall -std=c99
# fileno()
CFLAGS += -D_XOPEN_SOURCE
# vasprintf()
CFLAGS += -D_GNU_SOURCE
debug ?= no
ifeq ($(debug),yes)
	CFLAGS += -O0
else
	CFLAGS += -O2 -DNDEBUG
endif
LDLIBS = $(shell pkg-config --libs gtk+-3.0 glib-2.0 gstreamer-1.0)

sources = $(wildcard *.c)
no_objects = de.tab.o lex.yy.o
objects = $(filter-out $(no_objects), $(sources:.c=.o))
generated_parser_sources = de.tab.c lex.yy.c
resources = $(wildcard ../res/*)

prefix ?= $(abspath ../build)
bin_dir = $(prefix)/bin
share_dir = $(prefix)/share/$(bin)

.PHONY: all ctags clean move_resources

all: move_resources $(objects) $(generated_parser_sources)
	$(CC) $(CFLAGS) $(objects) $(generated_parser_sources) -o $(bin_dir)/$(bin) $(LDLIBS)

move_resources:
	mkdir -p $(share_dir)
	mkdir -p $(bin_dir)
	-cp $(resources) $(share_dir)
	sed -e 's/#define \(.*\) "\(.*\)"/#define \1 "$(subst /,\/,$(prefix))\/share\/$(bin)\/\2"/g' \
		config.h.orig > config.h

de.tab.c: de.y str.o
	bison -d $<

lex.yy.c: de.l
	flex $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDLIBS)

ctags:
	ctags -o ../tags -R --c++-kinds=+p --fields=+iaS --extra=+q \
		$(filter-out -p%, $(subst -I,, $(shell pkg-config --cflags gtk+-3.0 glib-2.0 gstreamer-1.0))) .

clean:
	-rm -f *.o $(generated_parser_sources) de.tab.h config.h
