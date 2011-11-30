SUBDIRS = theme

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = mutter-dawati.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

sessiondir = $(datadir)/xsessions
dist_session_DATA = dawati.desktop

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA)
else
install-data-local:
endif


CLEANFILES = $(schema_DATA)
EXTRA_DIST = $(schema_in_files)

include $(top_srcdir)/build/autotools/Makefile.am.panels
