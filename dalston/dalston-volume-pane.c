#include "dalston-volume-pane.h"
#include <libgvc/gvc-mixer-stream.h>
#include "dalston-volume-slider.h"
#include <glib/gi18n.h>
#include <canberra-gtk.h>
#include <nbtk/nbtk-gtk.h>
#include <gconf/gconf-client.h>

#define EVENT_SOUNDS_DIRECTORY "/desktop/gnome/sound"
#define EVENT_SOUNDS_KEY       EVENT_SOUNDS_DIRECTORY "/event_sounds"

G_DEFINE_TYPE (DalstonVolumePane, dalston_volume_pane, GTK_TYPE_HBOX)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_PANE, DalstonVolumePanePrivate))

typedef struct _DalstonVolumePanePrivate DalstonVolumePanePrivate;

struct _DalstonVolumePanePrivate {
  GConfClient *client;
  GvcMixerStream *sink;
  GtkWidget *mute_button;
  GtkWidget *alert_sounds_button;
  GtkWidget *test_sound_button;
  GtkWidget *volume_slider;
  guint gconf_connection_id;

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

#define TEST_SOUND_EVENT "audio-test-signal"

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

  if (priv->client)
  {
    if (priv->gconf_connection_id)
      {
        gconf_client_notify_remove (priv->client, priv->gconf_connection_id);
        priv->gconf_connection_id = 0;
      }
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->sink)
  {
    dalston_volume_pane_update_sink (pane, NULL);
  }

  G_OBJECT_CLASS (dalston_volume_pane_parent_class)->dispose (object);
}

static void
dalston_volume_pane_finalize (GObject *object)
{
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
_mute_button_switch_flipped_cb (NbtkGtkLightSwitch *light_switch,
                                gboolean            state,
                                DalstonVolumePane  *self)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (self);

  gvc_mixer_stream_change_is_muted (priv->sink, state);
  gtk_widget_set_sensitive (priv->volume_slider, !state);
}

static void
_alert_sounds_button_switch_flipped_cb (NbtkGtkLightSwitch *light_switch,
                                        gboolean            state,
                                        DalstonVolumePane  *self)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  gconf_client_set_bool (priv->client, EVENT_SOUNDS_KEY, state, &error);
  if (error)
  {
    g_warning (G_STRLOC " %s", error->message);
    g_clear_error (&error);
  }
}

static void
_set_alert_sounds_button_active (DalstonVolumePane *self,
                                 gboolean active)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (self);

  /* Block the emission of the switch-flipped signal */
  g_signal_handlers_block_by_func (priv->alert_sounds_button,
                                   _alert_sounds_button_switch_flipped_cb,
                                   self);

  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->alert_sounds_button),
                                    active);

  g_signal_handlers_unblock_by_func (priv->alert_sounds_button,
                                     _alert_sounds_button_switch_flipped_cb,
                                     self);
}

static void
_alert_sounds_button_update_cb (GConfClient *client,
                                guint cnxn_id,
                                GConfEntry *entry,
                                DalstonVolumePane *self)
{
  GConfValue *value = gconf_entry_get_value (entry);

  _set_alert_sounds_button_active (self,
                                   gconf_value_get_bool (value));
}

static void
dalston_volume_pane_init (DalstonVolumePane *self)
{
  DalstonVolumePanePrivate *priv = GET_PRIVATE (self);
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *align;
  GError    *error = NULL;
  gboolean   alert_sounds_button_active;
  gchar     *str;

  priv->client = gconf_client_get_default ();

  gtk_box_set_spacing (GTK_BOX (self), 4);
  gtk_container_set_border_width (GTK_CONTAINER (self), 4);

  frame = nbtk_gtk_frame_new ();
  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  str = g_strconcat ("<span font_desc=\"Liberation Sans Bold 18px\" foreground=\"#3e3e3e\">",
                     _("Output volume"),
                     "</span>",
                     NULL);
  label = gtk_label_new (str);
  g_free (str);

  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox),
                      label,
                      FALSE,
                      FALSE,
                      0);
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  priv->volume_slider = dalston_volume_slider_new ();
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->volume_slider,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (self),
                      frame,
                      TRUE,
                      TRUE,
                      0);

  frame = nbtk_gtk_frame_new ();
  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 32);
  gtk_table_set_row_spacings (GTK_TABLE (table), 16);
  gtk_container_set_border_width (GTK_CONTAINER (table), 8);
  gtk_container_add (GTK_CONTAINER (frame), table);

  align = gtk_alignment_new (0., 0.5, 0., 0.);
  gtk_table_attach (GTK_TABLE (table), align,
                    0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
                    0, 0);
  label = gtk_label_new (_("Mute"));
  gtk_container_add (GTK_CONTAINER (align), label);

  priv->mute_button = nbtk_gtk_light_switch_new ();
  g_signal_connect (priv->mute_button,
                    "switch-flipped",
                    (GCallback)_mute_button_switch_flipped_cb,
                    self);
  gtk_table_attach (GTK_TABLE (table), priv->mute_button,
                    1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
                    0, 0);

  align = gtk_alignment_new (0., 0.5, 0., 0.);
  gtk_table_attach (GTK_TABLE (table), align,
                    0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
                    0, 0);
  label = gtk_label_new (_("Alert sounds"));
  gtk_container_add (GTK_CONTAINER (align), label);

  priv->alert_sounds_button = nbtk_gtk_light_switch_new ();
  g_signal_connect (priv->alert_sounds_button,
                    "switch-flipped",
                    (GCallback)_alert_sounds_button_switch_flipped_cb,
                    self);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             priv->alert_sounds_button,
                             1, 2, 1, 2);

  priv->test_sound_button = gtk_button_new_with_label (_("Play test sound"));
  g_signal_connect (priv->test_sound_button,
                    "button-release-event",
                    (GCallback)_test_sound_button_release_cb,
                    self);
  gtk_table_attach_defaults (GTK_TABLE (table), priv->test_sound_button,
                             0, 2, 2, 3);

  gtk_box_pack_start (GTK_BOX (self),
                      frame,
                      FALSE,
                      FALSE,
                      0);

  priv->gconf_connection_id = gconf_client_notify_add (
                           priv->client,
                           EVENT_SOUNDS_DIRECTORY,
                           (GConfClientNotifyFunc)_alert_sounds_button_update_cb,
                           self,
                           NULL,
                           &error);
  if (error)
  {
    g_warning (G_STRLOC " %s", error->message);
    g_clear_error (&error);
  }

  gconf_client_add_dir (priv->client,
                        EVENT_SOUNDS_DIRECTORY,
                        GCONF_CLIENT_PRELOAD_ONELEVEL,
                        &error);
  if (error)
  {
    g_warning (G_STRLOC " %s", error->message);
    g_clear_error (&error);
  }

  alert_sounds_button_active = gconf_client_get_bool (priv->client,
                                                      EVENT_SOUNDS_KEY,
                                                      &error);
  if (error)
  {
    g_warning (G_STRLOC " %s", error->message);
    g_clear_error (&error);
  }
  _set_alert_sounds_button_active (self, alert_sounds_button_active);
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

  /* Block the emission of the switch-flipped signal */
  g_signal_handlers_block_by_func (priv->mute_button,
                                   _mute_button_switch_flipped_cb,
                                   pane);
  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->mute_button),
                                gvc_mixer_stream_get_is_muted (priv->sink));
  g_signal_handlers_unblock_by_func (priv->mute_button,
                                     _mute_button_switch_flipped_cb,
                                     pane);
  gtk_widget_set_sensitive (priv->volume_slider,
                            !gvc_mixer_stream_get_is_muted (priv->sink));
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
