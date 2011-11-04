QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	    $(PENGE_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir) \
	    -DLOCALEDIR=\"$(localedir)\" \
	    -DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	    -DTHEMEDIR=\"$(pkgdatadir)/theme\"

libexec_PROGRAMS = dawati-panel-myzone

dawati_panel_myzone_LDADD = \
			    $(top_builddir)/penge/libpenge.la \
			    $(PENGE_LIBS) \
			    $(MAILME_LIBS) \
			    $(MPL_LIBS)

dawati_panel_myzone_SOURCES = \
  dawati-panel-myzone.c

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.myzone.service.in
service_DATA = com.dawati.UX.Shell.Panels.myzone.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.myzone.service

com.dawati.UX.Shell.Panels.myzone.service: com.dawati.UX.Shell.Panels.myzone.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@
