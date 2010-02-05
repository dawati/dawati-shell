
bin_PROGRAMS=moblin-toolbar-properties

moblin_toolbar_properties_CFLAGS  = -I.. $(CAPPLET_CFLAGS) 	\
	-DPLUGIN_LOCALEDIR=\"$(localedir)\" 			\
	-DPANELSDIR=\"$(MUTTER_MOBLIN_PANELS_DIR)\" 		\
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\"

moblin_toolbar_properties_LDADD   = $(CAPPLET_LIBS)
moblin_toolbar_properties_SOURCES =		\
		mtp-jar.c			\
		mtp-jar.h			\
		mtp-toolbar.c			\
		mtp-toolbar.h			\
		mtp-toolbar-button.c		\
		mtp-toolbar-button.h		\
		mtp-space.c			\
		mtp-space.h			\
		moblin-toolbar-properties.c
