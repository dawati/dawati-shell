
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
# cannot be built (Penge calls into moblin_netbook_launch_*() functions, which
# dependency will go away when we move to the dbus API.) In the meantime,
#
# test-pasteboard: it uses some formating function from Penge utils; I just
#                c&p it into test-launcher.c

LDADD = \
	$(MUTTER_PLUGIN_LIBS)

noinst_PROGRAMS = \
       test-tray \
       test-pasteboard \
       test-screensized

test_tray_SOURCES = \
	test-tray.c

test_pasteboard_SOURCES = \
	$(top_srcdir)/src/mnb-clipboard-item.c \
	$(top_srcdir)/src/mnb-clipboard-store.c \
	$(top_srcdir)/src/mnb-clipboard-view.c \
	$(top_srcdir)/src/mnb-entry.c \
	$(top_builddir)/src/marshal.c \
	test-pasteboard.c

test_screensized_SOURCES = \
	test-screensized.c

