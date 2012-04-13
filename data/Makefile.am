SUBDIRS = theme
if MX_THEME
SUBDIRS += mx
endif

gsettings_SCHEMAS = org.dawati.shell.gschema.xml

@GSETTINGS_RULES@

sessiondir = $(datadir)/xsessions
dist_session_DATA = dawati.desktop

@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = $(schema_DATA)
EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels
