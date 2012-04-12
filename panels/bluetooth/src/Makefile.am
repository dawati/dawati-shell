libexec_PROGRAMS = dawati-panel-bluetooth

AM_CFLAGS = \
	$(LIBMPL_CFLAGS) \
	$(PANEL_BLUETOOTH_CFLAGS) \
	-I$(top_srcdir) \
	$(NULL)

BUILT_SOURCES = \
	dawati-bt-marshal.c \
	dawati-bt-marshal.h

dawati_panel_bluetooth_LDADD = \
	$(LIBMPL_LIBS) \
	$(PANEL_BLUETOOTH_LIBS) \
	$(BLUETOOTH_LIBS) \
	$(top_builddir)/carrick/libcarrick.la \
	$(NULL)

dawati_panel_bluetooth_SOURCES = \
	bluetooth-applet.h \
	dawati-bt-marshal.list \
	dawati-bt-device.c \
	dawati-bt-device.h \
	dawati-bt-request.c \
	dawati-bt-request.h \
	dawati-bt-shell.c \
	dawati-bt-shell.h \
	dawati-panel-bt.c \
	$(BUILT_SOURCES)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.bluetooth.service.in
service_DATA = com.dawati.UX.Shell.Panels.bluetooth.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.bluetooth.service

com.dawati.UX.Shell.Panels.bluetooth.service: com.dawati.UX.Shell.Panels.bluetooth.service.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

%-marshal.c: %-marshal.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=dawati_bt_marshal $< > $@

%-marshal.h: %-marshal.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=dawati_bt_marshal $< > $@
