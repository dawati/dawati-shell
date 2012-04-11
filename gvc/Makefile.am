privlibdir = $(pkglibdir)
privlib_LTLIBRARIES = libgvc.la


include $(top_srcdir)/build/autotools/Makefile.am.introspec

libgvc_la_CPPFLAGS =			\
	$(WARN_CFLAGS)			\
	$(GVC_CFLAGS)			\
	-DG_LOG_DOMAIN="\"Gvc\""

libgvc_la_LIBADD =			\
	$(GVC_LIBS)

libgvc_la_LDFLAGS = 			\
	-avoid-version

libgvc_la_gir_sources = \
	gvc-mixer-stream.h		\
	gvc-mixer-stream.c		\
	gvc-channel-map.h		\
	gvc-channel-map.c		\
	gvc-mixer-card.c		\
	gvc-mixer-card.h		\
	gvc-mixer-sink.h		\
	gvc-mixer-sink.c		\
	gvc-mixer-source.h		\
	gvc-mixer-source.c		\
	gvc-mixer-sink-input.h	\
	gvc-mixer-sink-input.c	\
	gvc-mixer-source-output.h	\
	gvc-mixer-source-output.c	\
	gvc-mixer-event-role.h	\
	gvc-mixer-event-role.c	\
	gvc-mixer-control.h		\
	gvc-mixer-control.c

libgvc_la_private_sources = 		\
	gvc-mixer-stream-private.h	\
	gvc-channel-map-private.h	\
	gvc-mixer-card-private.h	\
	gvc-mixer-control-private.h	\
	gvc-pulseaudio-fake.h

libgvc_la_SOURCES =			\
	$(libgvc_la_gir_sources)	\
	$(libgvc_la_private_sources)

Gvc-1.0.gir: libgvc.la
Gvc_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
Gvc_1_0_gir_CFLAGS = $(INCLUDES) -DWITH_INTROSPECTION
Gvc_1_0_gir_LIBS = libgvc.la
Gvc_1_0_gir_FILES = $(addprefix $(srcdir)/,$(libgvc_la_gir_sources))
INTROSPECTION_GIRS += Gvc-1.0.gir
CLEANFILES = Gvc-1.0.gir
