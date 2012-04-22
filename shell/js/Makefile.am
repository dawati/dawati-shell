EXTRA_DIST = misc/config.js.in
CLEANFILES = misc/config.js

misc/config.js: misc/config.js.in Makefile
	[ -d $(@D) ] || $(mkdir_p) $(@D) ; \
	sed -e "s|[@]PACKAGE_NAME@|$(PACKAGE_NAME)|g" \
	    -e "s|[@]PACKAGE_VERSION@|$(PACKAGE_VERSION)|g" \
	    -e "s|[@]GETTEXT_PACKAGE@|$(GETTEXT_PACKAGE)|g" \
	    -e "s|[@]datadir@|$(datadir)|g" \
	    -e "s|[@]libexecdir@|$(libexecdir)|g" \
	    -e "s|[@]sysconfdir@|$(sysconfdir)|g" \
	    -e "s|[@]DAWATI_PANELS_DIR@|$(DAWATI_PANELS_DIR)|g" \
               $< > $@

jsdir = $(pkgdatadir)/js
nobase_dist_js_DATA =	 		\
	misc/config.js			\
	misc/format.js			\
	misc/jsParse.js			\
	misc/params.js			\
	misc/screenSaver.js		\
	misc/util.js			\
					\
	ui/boxpointer.js		\
	ui/dateMenu.js			\
	ui/dnd.js			\
	ui/environment.js		\
	ui/layout.js			\
	ui/main.js			\
	ui/panel.js			\
	ui/panelMenu.js			\
	ui/popupMenu.js			\
	ui/status/accessibility.js	\
	ui/status/bluetooth.js		\
	ui/status/keyboard.js		\
	ui/status/power.js		\
	ui/status/volume.js		\
	ui/toolbar.js			\
	ui/tweener.js
