AM_CFLAGS = -Wall -Wextra -pedantic -std=c99
AM_CPPFLAGS =       					     \
	-D_GNU_SOURCE   					     \
	-D_XOPEN_SOURCE 					     \
	-DRESDIR=\"$(datadir)/$(PACKAGE_NAME)/\" \
	@AM_CPPFLAGS@
