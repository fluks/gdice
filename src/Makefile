bin := gdice
CC := gcc
CFLAGS := $(shell pkg-config --cflags gtk+-3.0 glib-2.0 gstreamer-1.0) \
	-g -pedantic -Wall -std=c99
# fileno()
CFLAGS += -D_XOPEN_SOURCE
# vasprintf()
CFLAGS += -D_GNU_SOURCE
DEBUG ?= no
ifeq ($(DEBUG), yes)
	CFLAGS += -O0
else
	CFLAGS += -O2 -DNDEBUG
endif
LDLIBS := $(shell pkg-config --libs gtk+-3.0 glib-2.0 gstreamer-1.0)

sources := $(wildcard *.c)
no_objects := de.tab.o lex.yy.o
objects := $(filter-out $(no_objects), $(sources:.c=.o))
generated_parser_sources := de.tab.c lex.yy.c
resources := $(wildcard ../res/*)

PREFIX := $(abspath ../build)
bin_dir := $(PREFIX)/bin
share_dir := $(PREFIX)/share/$(bin)

.PHONY: all ctags clean copy_resources create_config_h

all: copy_resources create_config_h $(objects) $(generated_parser_sources)
	$(CC) $(CFLAGS) $(objects) $(generated_parser_sources) -o $(bin_dir)/$(bin) $(LDLIBS)

copy_resources:
	mkdir -p $(share_dir)
	mkdir -p $(bin_dir)
	cp $(resources) $(share_dir)

create_config_h:
	sed -e 's/#define \(.*\) "\(.*\)"/#define \1 "$(subst /,\/,$(PREFIX))\/share\/$(bin)\/\2"/g' \
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
	-$(RM) *.o $(generated_parser_sources) de.tab.h config.h
