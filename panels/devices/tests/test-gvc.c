#include <libgvc/gvc-mixer-control.h>

typedef struct
{
  GvcMixerControl *control;
  GvcMixerStream  *default_sink;
} GvcTestApp;

static void
_stream_volume_notify_cb (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    userdata)
{
  g_debug (G_STRLOC ": Volume changed");
}

static void
_mixer_control_default_sink_changed_cb (GvcMixerControl *control,
                                        guint            id,
                                        gpointer         userdata)
{
  GvcTestApp *app = (GvcTestApp *)userdata;

  g_debug (G_STRLOC ": Default sink changed.");
  app->default_sink = gvc_mixer_control_get_default_sink (app->control);
  g_signal_connect (app->default_sink,
                    "notify::volume",
                    _stream_volume_notify_cb,
                    app);
}

static void
_mixer_control_ready_cb (GvcMixerControl *control,
                         gpointer         userdata)
{
  g_debug (G_STRLOC ": Ready.");
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  GvcTestApp *app;

  g_type_init ();

  app = g_new0 (GvcTestApp, 1);
  app->control = gvc_mixer_control_new ();
  g_signal_connect (app->control,
                    "default-sink-changed",
                    _mixer_control_default_sink_changed_cb,
                    app);
  g_signal_connect (app->control,
                    "ready",
                    _mixer_control_ready_cb,
                    app);
  gvc_mixer_control_open (app->control);

  loop = g_main_loop_new (NULL, TRUE);

  g_main_loop_run (loop);
  return 0;
}
