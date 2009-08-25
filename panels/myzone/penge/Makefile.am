lib_LTLIBRARIES = libpenge.la

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
		   penge-people-model.h

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
		      penge-people-model.c

AM_CFLAGS = -Wall \
	    $(PENGE_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir) \
	    -DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	    -DTHEMEDIR=\"$(pkgdatadir)/theme\"

libpenge_la_LIBADD = $(MPL_LIBS) \
		     $(PENGE_LIBS) 
