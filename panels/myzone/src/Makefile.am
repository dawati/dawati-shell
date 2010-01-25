QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	    $(PENGE_CFLAGS) \
	    $(MPL_CFLAGS) \
	    -I$(top_srcdir) \
	    -DLOCALEDIR=\"$(localedir)\" \
	    -DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	    -DTHEMEDIR=\"$(pkgdatadir)/theme\"

libexec_PROGRAMS = moblin-panel-myzone

moblin_panel_myzone_LDADD = \
			    $(top_builddir)/penge/libpenge.a \
			    $(PENGE_LIBS) \
			    $(MPL_LIBS)

moblin_panel_myzone_SOURCES = \
  moblin-panel-myzone.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.myzone.service.in
service_DATA = org.moblin.UX.Shell.Panels.myzone.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.myzone.service

org.moblin.UX.Shell.Panels.myzone.service: org.moblin.UX.Shell.Panels.myzone.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@
