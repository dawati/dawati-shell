QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

pluginsdir = $(datadir)/dawati-shell/plugins/
plugins_DATA = \
	example.js \
	example.plugin \
	$(NULL)

gsettings_SCHEMAS = \
	org.dawati.shell.home.plugin.example.gschema.xml \
	$(NULL)

@GSETTINGS_RULES@
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	$(plugins_DATA) \
	$(gsettings_SCHEMAS) \
	$(NULL)
