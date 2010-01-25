
AM_CFLAGS = \
	$(EGG_CONSOLE_KIT_CFLAGS) \
	-DEGG_CONSOLE="\"MPD_CONSOLE\"" \
	-DEGG_LOGGING="\"MPD_LOGGING\"" \
	-DEGG_LOG_FILE=\""/tmp/moblin-panel-devices.log"\" \
	-DEGG_VERBOSE="\"MPD_VERBOSE\"" \
	$(NULL)

noinst_LTLIBRARIES = libegg-console-kit.la

libegg_console_kit_la_SOURCES = \
	egg-console-kit.c \
	egg-console-kit.h \
	egg-debug.c \
	egg-debug.h \
	$(NULL)

