libexec_PROGRAMS = carrick-3g-wizard

carrick_3g_wizard_SOURCES = \
	main.c \
	ggg-service.h ggg-service.c \
	ggg-service-dialog.h ggg-service-dialog.c \
	ggg-country-dialog.h ggg-country-dialog.c \
	ggg-provider-dialog.h ggg-provider-dialog.c \
	ggg-plan-dialog.h ggg-plan-dialog.c \
	ggg-manual-dialog.h ggg-manual-dialog.c \
	ggg-mobile-info.h ggg-mobile-info.c \
	ggg-iso.h ggg-iso.c

carrick_3g_wizard_CFLAGS = -I$(top_srcdir) \
                           $(GTK_CFLAGS) \
                           $(DBUS_CFLAGS) \
                           $(REST_CFLAGS) \
                           -Wall

carrick_3g_wizard_LDADD= $(GTK_LIBS) $(DBUS_LIBS) $(REST_LIBS)

-include $(top_srcdir)/git.mk
