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
generated_parser_sources := de.tab.c lex.yy.c
objects := $(filter-out $(generated_parser_sources:.c=.o), $(sources:.c=.o))
resources := $(wildcard ../res/*)

PREFIX := $(abspath ../build)
prefix_file := prefix

goal := $(or $(findstring install, $(MAKECMDGOALS)), $(findstring uninstall, $(MAKECMDGOALS)))
ifneq ($(goal),)
	PREFIX := $(shell read -r line < $(prefix_file) && echo $$line)
	bin_dir := $(PREFIX)/bin
	share_dir := $(PREFIX)/share/$(bin)
endif

.PHONY: all ctags clean create_config_h install uninstall

all: create_config_h $(objects) $(generated_parser_sources)
	echo $(PREFIX) > $(prefix_file)
	$(CC) $(CFLAGS) $(objects) $(generated_parser_sources) -o ../$(bin) $(LDLIBS)

de.tab.c: de.y str.o
	bison -d $<

lex.yy.c: de.l
	flex $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDLIBS)

install:
	mkdir -p $(share_dir)
	cp $(resources) $(share_dir)
	mkdir -p $(bin_dir)
	cp ../$(bin) $(bin_dir)

uninstall:
	$(RM) $(share_dir)/*
	rmdir $(share_dir)
	$(RM) $(bin_dir)/*
	rmdir $(bin_dir)

ctags:
	ctags -o ../tags -R --c++-kinds=+p --fields=+iaS --extra=+q \
		$(filter-out -p%, $(subst -I,, $(shell pkg-config --cflags gtk+-3.0 glib-2.0 gstreamer-1.0))) .

clean:
	-$(RM) *.o $(generated_parser_sources) de.tab.h config.h $(prefix_file) ../$(bin)

create_config_h:
	sed -e 's/^#define \(.*\) "\(.*\)"$$/#define \1 "$(subst /,\/,$(PREFIX))\/share\/$(bin)\/\2"/g' \
		config.h.orig > config.h
