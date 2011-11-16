lib_LTLIBRARIES = libpenge.la
noinst_PROGRAMS = test-block-container test-everything-pane

pengeincludedir = $(pkgincludedir)/penge

pengeinclude_HEADERS = \
	penge-people-tile.h \
	penge-recent-file-tile.h \
	penge-event-tile.h \
	penge-events-pane.h \
	penge-calendar-pane.h \
	penge-grid-view.h \
	penge-utils.h \
	penge-magic-texture.h \
	penge-view-background.h \
	penge-apps-pane.h \
	penge-app-tile.h \
	penge-task-tile.h \
	penge-tasks-pane.h \
	penge-welcome-tile.h \
	penge-interesting-tile.h \
	penge-block-container.h \
	penge-everything-pane.h \
	penge-clickable-label.h \
	penge-email-pane.h \
	penge-count-tile.h \
	penge-dynamic-box.h \
	$(NULL)

libpenge_la_SOURCES = \
	$(pengeinclude_HEADERS) \
	penge-people-tile.c \
	penge-recent-file-tile.c \
	penge-event-tile.c \
	penge-events-pane.c \
	penge-calendar-pane.c \
	penge-grid-view.c \
	penge-utils.c \
	penge-magic-texture.c \
	penge-view-background.c \
	penge-apps-pane.c \
	penge-app-tile.c \
	penge-task-tile.c \
	penge-tasks-pane.c \
	penge-welcome-tile.c \
	penge-interesting-tile.c \
	penge-block-container.c \
	penge-everything-pane.c \
	penge-clickable-label.c \
	penge-email-pane.c \
	penge-count-tile.c \
	penge-dynamic-box.c \
	$(NULL)

libpenge_la_LIBADD = \
	$(PENGE_LIBS) \
	$(top_builddir)/libdawati-panel/dawati-panel/libdawati-panel.la \
	$(top_builddir)/mailme/libmailme.la \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	$(LIBMPL_CFLAGS) \
	$(PENGE_CFLAGS) \
	-I$(top_srcdir)/libdawati-panel \
	\
	-DTHEMEDIR=\"$(PANEL_MYZONE_THEMEDIR)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	$(NULL)

test_block_container_SOURCES = test-block-container.c
test_block_container_LDADD = \
	$(LIBMPL_LIBS) \
	$(PENGE_LIBS) \
	$(MAILME_LIBS) \
	$(top_builddir)/libdawati-panel/dawati-panel/libdawati-panel.la \
	libpenge.la \
	$(NULL)

test_everything_pane_SOURCES = test-everything-pane.c
test_everything_pane_LDADD = \
	$(LIBMPL_LIBS) \
	$(PENGE_LIBS) \
	$(MAILME_LIBS) \
	$(top_builddir)/libdawati-panel/dawati-panel/libdawati-panel.la \
	libpenge.la \
	$(NULL)
