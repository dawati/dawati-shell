@INTLTOOL_DESKTOP_RULE@

libexec_SCRIPTS = dawati-panel-music

jsfilesdir = $(PANEL_MUSIC_DATADIR)
dist_jsfiles_DATA = main.js \
	semantic.js \
	trackerResultSet.js \
	zeitgeist.js \
	$(NULL)

desktopfiles_in_files = dawati-panel-music.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

panelfilesdir = $(DAWATI_PANELS_DIR)
panelfiles_DATA = $(desktopfiles_files)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.music.service.in
service_DATA = com.dawati.UX.Shell.Panels.music.service

EXTRA_DIST = \
	dawati-panel-music.in \
	dawati-panel-music.desktop.in.in \
	$(service_in_files) \
	$(NULL)

CLEAN_FILES = \
	$(desktopfiles_in_files) \
	$(desktopfiles_files) \
	dawati-panel-music \
	$(NULL)

dawati-panel-music: dawati-panel-music.in
	$(AM_V_GEN) $(do_subst) $< > $@
	chmod +x $@

include $(top_srcdir)/build/autotools/Makefile.am.panels
