AM_CFLAGS = -Wall -Wextra -pedantic -std=c99
AM_CPPFLAGS =       					     \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-D_GNU_SOURCE   					     \
	-D_XOPEN_SOURCE 					     \
	-DRESDIR=\"$(datadir)/$(PACKAGE_NAME)/\" \
	@AM_CPPFLAGS@
