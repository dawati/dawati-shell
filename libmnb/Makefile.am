
noinst_LTLIBRARIES = libmnbgtk.la libmnbclutter.la

libmnbgtk_headers = mnb-panel-client.h mnb-panel-gtk.h

libmnbgtk_la_SOURCES = \
			$(libmnbgtk_headers) 	\
			mnb-panel-client.c 	\
			mnb-panel-gtk.c		\
			../src/marshal.c

libmnbgtk_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(GTK_CFLAGS)
libmnbgtk_la_LDFLAGS = $(LIBMNB_LIBS) $(GTK_LIBS)

libmnbclutter_headers = mnb-panel-client.h mnb-panel-clutter.h

libmnbclutter_la_SOURCES = \
			$(libmnbclutter_headers) 	\
			mnb-panel-client.c 		\
			mnb-panel-clutter.c		\
			../src/marshal.c

libmnbclutter_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(NBTK_CFLAGS)
libmnbclutter_la_LDFLAGS = $(LIBMNB_LIBS) $(NBTK_LIBS)
