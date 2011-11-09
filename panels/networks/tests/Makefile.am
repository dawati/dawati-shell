noinst_PROGRAMS = test-model

AM_CFLAGS = $(CARRICK_CFLAGS) -I$(top_srcdir)/carrick \
            $(GTK_CFLAGS) \
            $(DBUS_CFLAGS) -Wall \
	    -DLOCALEDIR=\""$(localedir)"\"

test_model_SOURCES = test-model.c
test_model_LDADD = $(top_builddir)/carrick/libcarrick.la \
		   $(GTK_LIBS) $(DBUS_LIBS)

-include $(top_srcdir)/git.mk
