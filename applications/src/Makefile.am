
AM_CFLAGS = \
  $(APPLICATIONS_CFLAGS) \
  -DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\"

bin_PROGRAMS = moblin-panel-applications

moblin_panel_applications_LDADD = $(APPLICATIONS_LIBS)

moblin_panel_applications_SOURCES = \
  mnb-launcher-application.c \
  mnb-launcher-application.h \
  mnb-launcher-button.c \
  mnb-launcher-button.h \
  mnb-launcher-grid.c \
  mnb-launcher-grid.h \
  mnb-launcher-tree.c \
  mnb-launcher-tree.h \
  moblin-netbook-launcher.c \
  moblin-netbook-launcher.h

