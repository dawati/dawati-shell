AM_CPPFLAGS = 						\
	-I.. $(CAPPLET_CFLAGS) 				\
	-DLIBEXECDIR=\"@libexecdir@\" 			\
	-DPKGLIBDIR=\"$(pkglibdir)\" 			\
	-DLOCALEDIR=\"$(localedir)\" 			\
	-DPLUGIN_LOCALEDIR=\"$(localedir)\" 		\
	-DPANELSDIR=\"$(DAWATI_PANELS_DIR)\" 	\
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\"	\
	-Wall -Wmissing-declarations

AM_LDFLAGS = \
	$(CAPPLET_LIBS)

lib_LTLIBRARIES = libmtp-common.la

libmtp_common_ladir = $(pkgincludedir)
libmtp_common_la_HEADERS = \
		mtp-jar.h			\
		mtp-toolbar.h			\
		mtp-toolbar-button.h		\
		mtp-clock.h			\
		mtp-space.h			\
		mtp-bin.h

libmtp_common_la_SOURCES = \
		mtp-jar.c			\
		mtp-toolbar.c			\
		mtp-toolbar-button.c		\
		mtp-clock.c			\
		mtp-space.c			\
		mtp-bin.c

bin_PROGRAMS = dawati-toolbar-properties

dawati_toolbar_properties_CFLAGS  = -I.. $(CAPPLET_CFLAGS) 	\
	-DPLUGIN_LOCALEDIR=\"$(localedir)\" 			\
	-DPANELSDIR=\"$(DAWATI_PANELS_DIR)\" 		\
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\"

dawati_toolbar_properties_LDADD   = libmtp-common.la
dawati_toolbar_properties_SOURCES = dawati-toolbar-properties.c

ccmodulesdir = $(CC_DIR)
ccmodules_LTLIBRARIES = libmtp.la libsystray.la # libapp.la

libmtp_la_SOURCES = \
	module.c \
	mtp-cc-panel.h \
	mtp-cc-panel.c
libmtp_la_CPPFLAGS = $(AM_CPPFLAGS) $(CC_CFLAGS)
libmtp_la_LIBADD = $(CC_LIBS) libmtp-common.la
libmtp_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version

libsystray_la_SOURCES = \
	syst-module.c \
	syst-cc-panel.h \
	syst-cc-panel.c
libsystray_la_CPPFLAGS = $(AM_CPPFLAGS) $(CAPPLET_CFLAGS)
libsystray_la_LIBADD = $(CAPPLET_LIBS)
libsystray_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version

#
#libapp_la_SOURCES = \
#	app-module.c \
#	app-cc-panel.h \
#	app-cc-panel.c
#libapp_la_CPPFLAGS = $(AM_CPPFLAGS) $(CAPPLET_CFLAGS)
#libapp_la_LIBADD = $(CAPPLET_LIBS)
#libapp_la_LDFLAGS = $(AM_LDFLAGS) --module --avoid-version
