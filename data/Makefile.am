SUBDIRS = theme

gsettings_SCHEMAS = org.dawati.shell.gschema.xml

@GSETTINGS_RULES@

sessiondir = $(datadir)/xsessions
dist_session_DATA = dawati.desktop

@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = $(schema_DATA)
EXTRA_DIST = $(schema_in_files)

include $(top_srcdir)/build/autotools/Makefile.am.panels
