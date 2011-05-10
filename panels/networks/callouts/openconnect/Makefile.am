libexec_PROGRAMS = carrick-openconnect-dialog

carrick_openconnect_dialog_SOURCES = \
	main.c auth-dlg-settings.h

carrick_openconnect_dialog_CFLAGS = -I$(top_srcdir) \
                                    $(GTHREAD_CFLAGS) \
                                    $(GTK_CFLAGS) \
                                    $(GCONF_CFLAGS) \
                                    $(OPENCONNECT_CFLAGS) \
                                    -Wall

carrick_openconnect_dialog_LDADD = $(GTK_LIBS) $(GTHREAD_LIBS) \
                                   $(GCONF_LIBS) $(OPENCONNECT_LIBS)

-include $(top_srcdir)/git.mk
