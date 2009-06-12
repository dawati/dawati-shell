
AM_CFLAGS = \
  $(APPLICATIONS_CFLAGS) \
  -I$(top_srcdir)/applications/src

LDADD = $(APPLICATIONS_LIBS)

noinst_PROGRAMS = \
	test-launcher-button \
	test-launcher-monitor \
	test-launcher-tree

test_launcher_button_SOURCES = \
	$(top_srcdir)/applications/src/mnb-launcher-button.c \
	test-launcher-button.c

test_launcher_monitor_SOURCES = \
	$(top_srcdir)/applications/src/mnb-launcher-application.c \
	$(top_srcdir)/applications/src/mnb-launcher-tree.c \
	test-launcher-monitor.c

test_launcher_tree_SOURCES = \
	$(top_srcdir)/applications/src/mnb-launcher-application.c \
	$(top_srcdir)/applications/src/mnb-launcher-tree.c \
	test-launcher-tree.c

