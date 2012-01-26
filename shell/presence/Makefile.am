noinst_LTLIBRARIES = libpresence.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@

libpresence_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libpresence_la_SOURCES = \
	mnb-presence.c		\
	mnb-presence.h		\
	gs-idle-monitor.c	\
	gs-idle-monitor.h	\
	gsm-manager.c		\
	gsm-manager.h		\
	gsm-presence.c		\
	gsm-presence.h		\
	gsm-store.c		\
	gsm-store.h		\
	gsm-inhibitor.c		\
	gsm-inhibitor.h

DBUS_GLUE = gsm-manager-glue.h gsm-inhibitor-glue.h gsm-presence-glue.h

gsm-manager-glue.h: org.gnome.SessionManager.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=gsm_manager --mode=glib-server --output=gsm-manager-glue.h $(srcdir)/org.gnome.SessionManager.xml

gsm-inhibitor-glue.h: org.gnome.SessionManager.Inhibitor.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=gsm_inhibitor --mode=glib-server --output=gsm-inhibitor-glue.h $(srcdir)/org.gnome.SessionManager.Inhibitor.xml

gsm-presence-glue.h: org.gnome.SessionManager.Presence.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=gsm_presence --mode=glib-server --output=gsm-presence-glue.h $(srcdir)/org.gnome.SessionManager.Presence.xml


BUILT_SOURCES = $(DBUS_GLUE)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = org.gnome.SessionManager.xml org.gnome.SessionManager.Presence.xml org.gnome.SessionManager.Inhibitor.xml
