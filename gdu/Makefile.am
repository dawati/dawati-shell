
AM_CFLAGS = \
  $(GDU_CFLAGS) \
  -I$(top_srcdir) \
  -DG_LOG_DOMAIN=\"moblin-panel-devices-gdu\" \
  -DGDU_API_IS_SUBJECT_TO_CHANGE \
  -DGDU_COMPILATION \
  $(NULL)

noinst_LTLIBRARIES = \
  libgdu.la \
  $(NULL)

libgdu_la_LIBADD = \
  $(GDU_LIBS) \
  $(NULL)

libgdu_la_SOURCES = \
  $(BUILT_SOURCES) \
  gdu-adapter.c \
  gdu-adapter.h \
  gdu-callbacks.h \
  gdu-device.c \
  gdu-device.h \
  gdu-drive.c \
  gdu-drive.h \
  gdu-error.c \
  gdu-error.h \
  gdu-expander.c \
  gdu-expander.h \
  gdu.h \
  gdu-hub.c \
  gdu-hub.h \
  gdu-known-filesystem.c \
  gdu-known-filesystem.h \
  gdu-linux-lvm2-volume.c \
  gdu-linux-lvm2-volume-group.c \
  gdu-linux-lvm2-volume-group.h \
  gdu-linux-lvm2-volume.h \
  gdu-linux-lvm2-volume-hole.c \
  gdu-linux-lvm2-volume-hole.h \
  gdu-linux-md-drive.c \
  gdu-linux-md-drive.h \
  gdu-machine.c \
  gdu-machine.h \
  gdu-pool.c \
  gdu-pool.h \
  gdu-port.c \
  gdu-port.h \
  gdu-presentable.c \
  gdu-presentable.h \
  gdu-private.h \
  gdu-process.c \
  gdu-process.h \
  gdu-ssh-bridge.c \
  gdu-ssh-bridge.h \
  gdu-types.h \
  gdu-util.c \
  gdu-util.h \
  gdu-volume.c \
  gdu-volume.h \
  gdu-volume-hole.c \
  gdu-volume-hole.h \
  $(NULL)

BUILT_SOURCES = \
  udisks-daemon-glue.h \
  udisks-device-glue.h \
  udisks-adapter-glue.h \
  udisks-expander-glue.h \
  udisks-port-glue.h \
  gdu-marshal.h \
  gdu-marshal.c \
  $(NULL)

DISTCLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
  gdu.pc.in \
  gdu-marshal.list \
  $(NULL)

gdu-marshal.h: gdu-marshal.list
	$(AM_V_GEN)glib-genmarshal $< --prefix=gdu_marshal --header > $@

gdu-marshal.c: gdu-marshal.list
	$(AM_V_GEN)echo "#include \"gdu-marshal.h\"" > $@ && glib-genmarshal $< --prefix=gdu_marshal --body >> $@

udisks-daemon-glue.h: $(UDISKS_DATADIR)/dbus-1/interfaces/org.freedesktop.UDisks.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=$@ $<

udisks-device-glue.h: $(UDISKS_DATADIR)/dbus-1/interfaces/org.freedesktop.UDisks.Device.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=$@ $<

udisks-adapter-glue.h: $(UDISKS_DATADIR)/dbus-1/interfaces/org.freedesktop.UDisks.Adapter.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=$@ $<

udisks-expander-glue.h: $(UDISKS_DATADIR)/dbus-1/interfaces/org.freedesktop.UDisks.Expander.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=$@ $<

udisks-port-glue.h: $(UDISKS_DATADIR)/dbus-1/interfaces/org.freedesktop.UDisks.Port.xml Makefile.am
	$(AM_V_GEN)dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=$@ $<

