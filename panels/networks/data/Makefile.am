SUBDIRS = icons theme

paneldir = @PANEL_DIR@
panel_in_files = carrick.desktop.in.in
panel_DATA = $(panel_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(panel_in_files)
CLEANFILES = $(panel_DATA)

install-data-hook:
	mkdir --parents $(DESTDIR)$(sysconfdir)/xdg/autostart
	ln -s $(DESTDIR)$(paneldir)/$(panel_DATA) $(DESTDIR)$(sysconfdir)/xdg/autostart

-include $(top_srcdir)/git.mk
