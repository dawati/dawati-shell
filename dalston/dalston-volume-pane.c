#include "dalston-volume-pane.h"
#include <libgvc/gvc-mixer-stream.h>
#include "dalston-volume-slider.h"
#include <glib/gi18n.h>
#include <canberra-gtk.h>

G_DEFINE_TYPE (DalstonVolumePane, dalston_volume_pane, GTK_TYPE_HBOX)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_PANE, DalstonVolumePanePrivate))

typedef struct _DalstonVolumePanePrivate DalstonVolumePanePrivate;

struct _DalstonVolumePanePrivate {
  GvcMixerStream *sink;
  GtkWidget *mute_button;
  GtkWidget *test_sound_button;
  GtkWidget *volume_slider;

#if 0
  ca_context *ca_context;
#endif
};

enum
{
  PROP_0,
  PROP_SINK
};

static void dalston_volume_pane_update_sink (DalstonVolumePane *pane,
                                             GvcMixerStream    *new_sink);

#define TEST_SOUND_EVENT "desktop-login"

static void
dalston_volume_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SINK:
      g_value_set_object (value, priv->sink);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DalstonVolumePane *pane = (DalstonVolumePane *)object;
  GvcMixerStream *sink;

  switch (property_id) {
    case PROP_SINK:
      sink = (GvcMixerStream *)g_value_get_object (value);
      dalston_volume_pane_update_sink (pane, sink);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_pane_dispose (GObject *object)
{
  DalstonVolumePane *pane = (DalstonVolumePane *)object;
  DalstonVolumePanePrivate *priv = GET_PRIVATE (pane);

  if (priv->sink)
  {
    dalston_volume_pane_update_sink (pane, NULL);
  }

  G_OBJECT_CLASS (dalston_volume_pane_parent_class)->dispose (object);
}

static void
dalston_volume_pane_finalize (GObject *object)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (dalston_volume_pane_parent_class)->finalize (object);
}

static void
dalston_volume_pane_class_init (DalstonVolumePaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DalstonVolumePanePrivate));

  object_class->get_property = dalston_volume_pane_get_property;
  object_class->set_property = dalston_volume_pane_set_property;
  object_class->dispose = dalston_volume_pane_dispose;
  object_class->finalize = dalston_volume_pane_finalize;

  pspec = g_param_spec_object ("sink",
                               "Sink.",
                               "The sink to use.",
                               GVC_TYPE_MIXER_STREAM,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SINK, pspec);
}

static gboolean
_test_sound_button_release_cb (GtkWidget      *widget,
                                    GdkEventButton *button_event,
                                    gpointer        userdata)
{
  gint res;
  res = ca_gtk_play_for_event ((GdkEvent *)button_event,
                               0,
                               CA_PROP_EVENT_ID,
                               TEST_SOUND_EVENT,
                               NULL);

  if (res != CA_SUCCESS)
  {
    g_warning (G_STRLOC ": Error playing test sound: %s",
               ca_strerror (res));
  }

  return FALSE;
}

static void
_mute_button_toggled_cb (GtkToggleButton *toggle_button,
                         gpointer         userdata)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (userdata);
  gboolean toggled;

  toggled = gtk_toggle_button_get_active (toggle_button);
  gvc_mixer_stream_change_is_muted (priv->sink, toggled);
}

static void
dalston_volume_pane_init (DalstonVolumePane *self)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (self);
  GtkWidget *vbox;

  priv->volume_slider = dalston_volume_slider_new ();
  gtk_box_pack_start (GTK_BOX (self),
                      priv->volume_slider,
                      TRUE,
                      TRUE,
                      8);
  vbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (self),
                      vbox,
                      FALSE,
                      FALSE,
                      8);
  priv->mute_button = gtk_toggle_button_new_with_label (_("Mute"));
  g_signal_connect (priv->mute_button,
                    "toggled",
                    (GCallback)_mute_button_toggled_cb,
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->mute_button,
                      TRUE,
                      FALSE,
                      8);
  priv->test_sound_button = gtk_button_new_with_label (_("Play test sound"));
  g_signal_connect (priv->test_sound_button,
                    "button-release-event",
                    (GCallback)_test_sound_button_release_cb,
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->test_sound_button,
                      TRUE,
                      FALSE,
                      8);
}

GtkWidget *
dalston_volume_pane_new (void)
{
  return g_object_new (DALSTON_TYPE_VOLUME_PANE, NULL);
}

static void
dalston_volume_pane_update_mute (DalstonVolumePane *pane)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (pane);

  /* Block the emission of the toggled signal */
  g_signal_handlers_block_by_func (priv->mute_button,
                                   _mute_button_toggled_cb,
                                   pane);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->mute_button),
                                gvc_mixer_stream_get_is_muted (priv->sink));
  g_signal_handlers_unblock_by_func (priv->mute_button,
                                     _mute_button_toggled_cb,
                                     pane);
}

static void
_stream_is_muted_notify_cb (GObject    *object,
                            GParamSpec *pspec,
                            gpointer    userdata)
{
  dalston_volume_pane_update_mute ((DalstonVolumePane *)userdata);
}

static void
dalston_volume_pane_update_sink (DalstonVolumePane *pane,
                                 GvcMixerStream    *new_sink)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (pane);

  if (priv->sink)
  {
    g_signal_handlers_disconnect_by_func (priv->sink,
                                          _stream_is_muted_notify_cb,
                                          pane);
    g_object_unref (priv->sink);
    priv->sink = NULL;
  }

  if (new_sink)
  {
    priv->sink = g_object_ref (new_sink);

    g_signal_connect (priv->sink,
                      "notify::is-muted",
                      (GCallback)_stream_is_muted_notify_cb,
                      pane);
    dalston_volume_pane_update_mute (pane);
  }

  g_object_set (priv->volume_slider,
                "sink",
                priv->sink,
                NULL);
}
