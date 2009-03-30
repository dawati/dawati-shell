noinst_LTLIBRARIES = libgvc.la
libgvc_la_CFLAGS = $(PULSE_CFLAGS) \
		   -DPKG_DATADIR=\"$(pkgdatadir)\" -Wall
libgvc_la_LIBADD = $(PULSE_LIBS) $(GLIB_LIBS)
libgvc_la_SOURCES = gvc-channel-map.c  \
		    gvc-channel-map.h  \
		    gvc-mixer-control.c \
		    gvc-mixer-control.h \
		    gvc-mixer-event-role.c \
		    gvc-mixer-event-role.h \
		    gvc-mixer-sink.c  \
		    gvc-mixer-sink.h  \
		    gvc-mixer-stream.c \
		    gvc-mixer-stream.h \
		    gvc-mixer-source-output.c \
		    gvc-mixer-source-output.h \
		    gvc-mixer-sink-input.c \
		    gvc-mixer-sink-input.h \
		    gvc-mixer-source.c \
		    gvc-mixer-source.h

