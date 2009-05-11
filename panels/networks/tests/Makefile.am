noinst_PROGRAMS = test-dnd test-applet test-icon-factory
# test-service-list
#
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

test_icon_factory_SOURCES = test-icon-factory.c
test_icon_factory_LDADD = $(top_builddir)/carrick/libcarrick.la \
			  $(GTK_LIBS) $(GCONNMAN_LIBS)

-include $(top_srcdir)/git.mk
