resdir = $(datadir)/@PACKAGE_NAME@
dist_res_DATA = \
	gdice.svg \
	add_12x12.svg \
	dices.ogg \
	gdice.glade \
	remove_12x12.svg \
	gdice.css

desktopdir = $(datadir)/applications
desktop_in_files = gdice.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

icondir = $(datadir)/pixmaps
icon_DATA = gdice.svg

gsettings_SCHEMAS = com.github.fluks.GDice.gschema.xml
EXTRA_DIST = $(gsettings_SCHEMAS)
@GSETTINGS_RULES@
