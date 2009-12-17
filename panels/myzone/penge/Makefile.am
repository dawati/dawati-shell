lib_LTLIBRARIES = libpenge.la
noinst_PROGRAMS = test-block-container test-everything-pane

libpenge_headers = penge-people-pane.h \
		   penge-people-tile.h \
		   penge-recent-files-pane.h \
		   penge-recent-file-tile.h \
		   penge-date-tile.h \
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
		   penge-magic-container.h \
		   penge-magic-list-view.h \
		   penge-people-model.h \
		   penge-recent-files-model.h \
		   penge-welcome-tile.h \
		   penge-people-placeholder-tile.h \
		   penge-interesting-tile.h \
		   penge-source-manager.h \
		   penge-block-container.h \
		   penge-model-bridge.h \
		   penge-everything-pane.h

libpenge_la_SOURCES = \
		      $(libpenge_headers) \
		      penge-people-pane.c \
		      penge-people-tile.c \
		      penge-recent-files-pane.c \
		      penge-recent-file-tile.c \
		      penge-date-tile.c \
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
		      penge-magic-container.c \
		      penge-magic-list-view.c \
		      penge-people-model.c \
		      penge-recent-files-model.c \
		      penge-welcome-tile.c \
		      penge-people-placeholder-tile.c \
		      penge-interesting-tile.c \
		      penge-source-manager.c \
		      penge-block-container.c \
		      penge-model-bridge.c \
		      penge-everything-pane.c

AM_CFLAGS = -Wall \
	    $(PENGE_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir) \
	    -DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	    -DTHEMEDIR=\"$(pkgdatadir)/theme\"

libpenge_la_LIBADD = $(MPL_LIBS) \
		     $(PENGE_LIBS)

test_block_container_SOURCES = test-block-container.c
test_block_container_LDADD = libpenge.la

test_everything_pane_SOURCES = test-everything-pane.c
test_everything_pane_LDADD = libpenge.la

