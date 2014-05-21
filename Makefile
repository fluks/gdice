src_dir = src/

.PHONY: all ctags clean

all:
	$(MAKE) -C $(src_dir) $@

ctags:
	$(MAKE) -C $(src_dir) $@

clean:
	$(MAKE) -C $(src_dir) $@
