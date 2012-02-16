AM_CFLAGS = \
	$(LIBMPL_CFLAGS) \
	$(PANEL_APPLICATIONS_CFLAGS) \
	-I$(srcdir)/../src \
	$(NULL)

LDADD = \
	$(LIBMPL_LIBS) \
	$(PANEL_APPLICATIONS_LIBS) \
	$(NULL)

noinst_PROGRAMS = \
	test-launcher-button \
	test-launcher-monitor \
	test-launcher-tree

test_launcher_button_SOURCES = \
	$(srcdir)/../src/mnb-launcher-button.c \
	test-launcher-button.c

test_launcher_monitor_SOURCES = \
	$(srcdir)/../src/mnb-launcher-application.c \
	$(srcdir)/../src/mnb-launcher-tree.c \
	test-launcher-monitor.c

test_launcher_tree_SOURCES = \
	$(srcdir)/../src/mnb-launcher-application.c \
	$(srcdir)/../src/mnb-launcher-tree.c \
	test-launcher-tree.c

