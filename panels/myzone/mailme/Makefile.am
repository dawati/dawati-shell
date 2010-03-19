noinst_LTLIBRARIES = libmailme.la
noinst_PROGRAMS = test-mailme

libmailme_la_headers = mailme-telepathy.h \
		       mailme-telepathy-account.h

libmailme_la_SOURCES = \
		      $(libmailme_la_headers) \
		      mailme-telepathy.c \
		      mailme-telepathy-account.c

AM_CFLAGS = -Wall \
	    $(MAILME_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir)

test_mailme_SOURCES = test-mailme.c
test_mailme_LDADD = libmailme.la $(MAILME_LIBS)
