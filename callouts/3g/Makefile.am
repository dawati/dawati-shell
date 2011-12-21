libexec_PROGRAMS = carrick-3g-wizard

carrick_3g_wizard_SOURCES = \
	main.c \
	ggg-sim-dialog.h ggg-sim-dialog.c \
	ggg-country-dialog.h ggg-country-dialog.c \
	ggg-provider-dialog.h ggg-provider-dialog.c \
	ggg-plan-dialog.h ggg-plan-dialog.c \
	ggg-manual-dialog.h ggg-manual-dialog.c \
	ggg-mobile-info.h ggg-mobile-info.c \
	ggg-iso.h ggg-iso.c \
	ggg-sim.h ggg-sim.c \
	$(NULL)

carrick_3g_wizard_CFLAGS = \
	-I$(top_srcdir) \
	$(CALLOUTS_CFLAGS) \
	-Wall

carrick_3g_wizard_LDADD = \
	$(CALLOUTS_LIBS) \
	$(NULL)

-include $(top_srcdir)/git.mk
