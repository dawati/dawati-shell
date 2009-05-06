
noinst_LTLIBRARIES = libmnb.la

libmnb_headers = mnb-panel-client.h

libmnb_la_SOURCES = \
			$(libmnb_headers) 	\
			mnb-panel-client.c 	\
			../src/marshal.c

AM_CFLAGS = -Wall $(LIBMNB_CFLAGS) \
	    -DPKG_DATADIR=\"$(pkgdatadir)\"
AM_LDFLAGS = $(LIBMNB_LIBS)
