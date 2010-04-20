AM_CFLAGS = \
	$(APPLICATIONS_CFLAGS) \
	-I$(top_srcdir)/src \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\" \
	$(NULL)

LDADD = $(APPLICATIONS_LIBS)

noinst_PROGRAMS = \
	test-filter \
	test-launcher-button \
	test-launcher-monitor \
	test-launcher-tree

test_filter_SOURCES = \
	$(top_srcdir)/src/mnb-filter.c \
	test-filter.c

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_launcher_monitor_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-application.c \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-monitor.c

test_launcher_tree_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-application.c \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-tree.c

