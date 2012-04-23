QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.home.service.in
service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in Makefile.am
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

gsettings_SCHEMAS = \
	org.dawati.shell.home.gschema.xml \
	org.dawati.shell.home.plugin.gschema.xml \
	$(NULL)

@GSETTINGS_RULES@
@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = \
	$(service_DATA) \
	$(schema_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(service_in_files) \
	$(gsettings_SCHEMAS) \
	$(NULL)
