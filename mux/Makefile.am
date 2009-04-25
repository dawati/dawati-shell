noinst_LTLIBRARIES = libmux.la

libmux_la_SOURCES = mux-light-switch.c \
                    mux-light-switch.h \
                    mux-switch-box.c \
                    mux-switch-box.h \
                    mux-cell-renderer-text.c \
                    mux-cell-renderer-text.h \
                    mux-bin.c \
                    mux-bin.h

libmux_la_CFLAGS = $(GTK_CFLAGS)

libmux_la_LIBADD = $(GTK_LIBS)

-include $(top_srcdir)/git.mk
