noinst_LTLIBRARIES = libpresence.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@

libpresence_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libpresence_la_SOURCES = \
	mnb-presence.c		\
	mnb-presence.h		\
	gs-idle-monitor.c	\
	gs-idle-monitor.h	\
	gsm-presence.c		\
	gsm-presence.h

DBUS_GLUE = gsm-presence-glue.h

gsm-presence-glue.h: org.gnome.SessionManager.Presence.xml Makefile.am
	dbus-binding-tool --prefix=gsm_presence --mode=glib-server --output=gsm-presence-glue.h $(srcdir)/org.gnome.SessionManager.Presence.xml

BUILT_SOURCES = gsm-presence-glue.h
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = org.gnome.SessionManager.Presence.xml
