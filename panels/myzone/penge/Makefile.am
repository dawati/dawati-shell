noinst_LIBRARIES = libpenge.a
noinst_PROGRAMS = test-block-container test-everything-pane
libpenge_a_headers = \
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
		   penge-people-placeholder-tile.h \
		   penge-interesting-tile.h \
		   penge-block-container.h \
		   penge-everything-pane.h \
		   penge-clickable-label.h

libpenge_a_SOURCES = \
		      $(libpenge_a_headers) \
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
		      penge-people-placeholder-tile.c \
		      penge-interesting-tile.c \
		      penge-block-container.c \
		      penge-everything-pane.c \
		      penge-clickable-label.c

AM_CFLAGS = -Wall \
	    $(PENGE_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir) \
	    -DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	    -DTHEMEDIR=\"$(pkgdatadir)/theme\"

test_block_container_SOURCES = test-block-container.c
test_block_container_LDADD = libpenge.a \
			     $(MPL_LIBS) \
			     $(PENGE_LIBS)

test_everything_pane_SOURCES = test-everything-pane.c
test_everything_pane_LDADD = libpenge.a \
			     $(MPL_LIBS) \
			     $(PENGE_LIBS)

