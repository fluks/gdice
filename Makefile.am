SUBDIRS = \
	res \
	src

my_ctags:
	ctags -o tags -R --c++-kinds=+p --fields=+iaS --extra=+q \
		$(filter-out -p%, $(subst -I,, $(AM_CPPFLAGS))) \
		src
