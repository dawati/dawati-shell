noinst_PROGRAMS = test-applet

AM_CFLAGS = $(CARRICK_CFLAGS) -I$(top_srcdir)/carrick \
            $(GTK_CFLAGS) \
            $(GCONNMAN_CFLAGS) -Wall \
	    -DLOCALEDIR=\""$(localedir)"\"

test_applet_SOURCES = test-applet.c
test_applet_LDADD = $(top_builddir)/carrick/libcarrick.la \
                    $(GTK_LIBS) $(GCONNMAN_LIBS)

-include $(top_srcdir)/git.mk
