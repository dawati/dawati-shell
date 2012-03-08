QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.home.service.in
service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in Makefile.am
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

CLEANFILES = \
	$(service_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(service_in_files) \
	$(NULL)
