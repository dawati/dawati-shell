noinst_PROGRAMS = test-dnd test-service-list test-applet
AM_CFLAGS = $(CARRICK_CFLAGS) -I$(top_srcdir)/carrick \
            $(GTK_CFLAGS) \
            $(GCONNMAN_CFLAGS) -Wall

test_dnd_SOURCES = test-dnd.c
test_dnd_LDADD = $(top_builddir)/carrick/libcarrick.la \
                 $(GTK_LIBS)


test_service_list_SOURCES = test-service-list.c
test_service_list_LDADD = $(top_builddir)/carrick/libcarrick.la \
                          $(GTK_LIBS) $(GCONNMAN_LIBS)

test_applet_SOURCES = test-applet.c
test_applet_LDADD = $(top_builddir)/carrick/libcarrick.la \
                    $(GTK_LIBS) $(GCONNMAN_LIBS)

test_mux_switch_SOURCES = test-mux-switch.c
test_mux_switch_LDADD = $(top_builddir)/mux/libmux.la \
			$(GTK_LIBS)

-include $(top_srcdir)/git.mk
