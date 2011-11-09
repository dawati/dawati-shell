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
		   penge-dynamic-box.h

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
		      penge-dynamic-box.c

libpenge_la_LIBADD = $(top_builddir)/mailme/libmailme.la

AM_CFLAGS = -Wall \
	    $(PENGE_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir) \
	    -DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	    -DTHEMEDIR=\"$(pkgdatadir)/theme\" \
	    -DLOCALEDIR=\"$(localedir)\"

test_block_container_SOURCES = test-block-container.c
test_block_container_LDADD = libpenge.la \
			     $(MPL_LIBS) \
			     $(PENGE_LIBS) \
			     $(MAILME_LIBS)



test_everything_pane_SOURCES = test-everything-pane.c
test_everything_pane_LDADD = libpenge.la \
			     $(MPL_LIBS) \
			     $(PENGE_LIBS) \
			     $(MAILME_LIBS)

