src_dir = src/

.PHONY: all ctags clean install uninstall

all:
	$(MAKE) -C $(src_dir) $@ $(MAKEFLAGS)

ctags:
	$(MAKE) -C $(src_dir) $@

clean:
	$(MAKE) -C $(src_dir) $@

install:
	$(MAKE) -C $(src_dir) $@

uninstall:
	$(MAKE) -C $(src_dir) $@
