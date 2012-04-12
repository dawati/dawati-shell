
AM_CPPFLAGS = \
	$(DAWATI_SHELL_CFLAGS) \
	-I$(top_srcdir)/penge \
	-I$(top_srcdir)/shell \
	-I$(top_builddir)/shell \
	-DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\" \
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\" \
	-DPANELSDIR=\"$(DAWATI_PANELS_DIR)\" \
	$(PENGE_CFLAGS) \
	-fPIC \
	-Werror=missing-prototypes

# FIXME -- because of various fscked dependencies any tests relying on Penge
# cannot be built (Penge calls into dawati_netbook_launch_*() functions, which
# dependency will go away when we move to the dbus API.) In the meantime,
#
# test-pasteboard: it uses some formating function from Penge utils; I just
#                c&p it into test-launcher.c

LDADD = \
	$(DAWATI_SHELL_LIBS)

noinst_PROGRAMS = \
	test-screensized \
	test-statusbar

test_screensized_SOURCES = \
	test-screensized.c

test_statusbar_SOURCES = \
	test-statusbar.c \
	$(top_srcdir)/shell/mnb-statusbar.c
test_statusbar_CFLAGS = \
	-I$(top_srcdir)/shell \
	-I$(top_srcdir)/libdawati-panel
