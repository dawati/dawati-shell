
AM_CPPFLAGS = \
  $(MUTTER_PLUGIN_CFLAGS) \
  -I$(top_srcdir)/penge \
  -I$(top_srcdir)/src \
  -I$(top_builddir)/src \
  -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = \
	$(PENGE_CFLAGS) \
	-fPIC \
	-Werror=missing-prototypes

# FIXME -- because of various fscked dependencies any tests relying on Penge
# cannot be built (Penge calls into meego_netbook_launch_*() functions, which
# dependency will go away when we move to the dbus API.) In the meantime,
#
# test-pasteboard: it uses some formating function from Penge utils; I just
#                c&p it into test-launcher.c

LDADD = \
	$(MUTTER_PLUGIN_LIBS)

noinst_PROGRAMS = \
       test-screensized

test_screensized_SOURCES = \
	test-screensized.c

