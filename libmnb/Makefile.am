
noinst_LTLIBRARIES = libmnbgtk.la

libmnbgtk_headers = mnb-panel-client.h

libmnbgtk_la_SOURCES = \
			$(libmnb_headers) 	\
			mnb-panel-client.c 	\
			mnb-panel-gtk.c		\
			../src/marshal.c

libmnbgtk_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(GTK_CFLAGS)

libmnbgtk_la_LDFLAGS = $(LIBMNB_LIBS) $(GTK_LIBS)
