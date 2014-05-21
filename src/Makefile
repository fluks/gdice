BIN = gtk_dice
CC = gcc
# Append flags(possibly overriding existing ones) with make CFLAGS=<flags> ...
override CFLAGS := `pkg-config --cflags gtk+-2.0` -g3 -O2 -pedantic -Wall -Werror -std=c99 $(CFLAGS)
LDLIBS = `pkg-config --libs gtk+-2.0` -lportaudio
OBJECTS = main.o gui.o sound.o 
CTAGS_OUTPUT = tags

.PHONY: all ctags clean

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDLIBS) $(OBJECTS) -o $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) $(LDLIBS) -c -o $@ $<

ctags:
	ctags -R -o $(CTAGS_OUTPUT) --c++-kinds=+p --fields=+iaS --extra=+q /usr/include/portaudio.h /usr/include/gtk-2.0/ /usr/lib/gtk-2.0/include/ /usr/include/glib-2.0/ /usr/lib/glib-2.0/include/ .

clean:
	@rm -f $(OBJECTS) $(CTAGS_OUTPUT) 
