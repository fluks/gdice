src_dir = src/

.PHONY: all ctags clean install uninstall

all:
	$(MAKE) -C $(src_dir) $@ $(MAKEFLAGS)

ctags:
	ctags -o tags -R --c++-kinds=+p --fields=+iaS --extra=+q \
		$(filter-out -p%, $(subst -I,, $(shell pkg-config --cflags gtk+-3.0 glib-2.0 gstreamer-1.0))) $(src_dir)
clean:
	$(MAKE) -C $(src_dir) $@

install:
	$(MAKE) -C $(src_dir) $@

uninstall:
	$(MAKE) -C $(src_dir) $@
