#include "dalston-volume-slider.h"

#include <libgvc/gvc-mixer-stream.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (DalstonVolumeSlider, dalston_volume_slider, GTK_TYPE_HSCALE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_SLIDER, DalstonVolumeSliderPrivate))

typedef struct _DalstonVolumeSliderPrivate DalstonVolumeSliderPrivate;

struct _DalstonVolumeSliderPrivate {
  GvcMixerStream *sink;
};

enum
{
  PROP_0,
  PROP_SINK
};

static void dalston_volume_slider_update_sink (DalstonVolumeSlider *slider,
                                               GvcMixerStream      *new_sink);


static void _stream_volume_notify_cb (GObject    *object,
                                      GParamSpec *pspec,
                                      gpointer    userdata);

static void
dalston_volume_slider_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DalstonVolumeSliderPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SINK:
      g_value_set_object (value, priv->sink);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_slider_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DalstonVolumeSlider *slider = (DalstonVolumeSlider *)object;
  GvcMixerStream *sink;

  switch (property_id) {
    case PROP_SINK:
      sink = (GvcMixerStream *)g_value_get_object (value);
      dalston_volume_slider_update_sink (slider, sink);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_slider_dispose (GObject *object)
{
  dalston_volume_slider_update_sink ((DalstonVolumeSlider *)object, NULL);

  G_OBJECT_CLASS (dalston_volume_slider_parent_class)->dispose (object);
}

static void
dalston_volume_slider_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_volume_slider_parent_class)->finalize (object);
}

static void
dalston_volume_slider_constructed (GObject *object)
{
  DalstonVolumeSlider *slider = (DalstonVolumeSlider *)object;

  gtk_range_set_range (GTK_RANGE (slider),
                       0.0,
                       100.0);
  gtk_range_set_increments (GTK_RANGE (slider),
                            5,
                            20);
  gtk_scale_set_digits (GTK_SCALE (slider),
                        0);

  gtk_range_set_restrict_to_fill_level (GTK_RANGE (slider),
                                        FALSE);
  gtk_range_set_show_fill_level (GTK_RANGE (slider),
                                 TRUE);

  if (G_OBJECT_CLASS (dalston_volume_slider_parent_class)->constructed)
  {
    G_OBJECT_CLASS (dalston_volume_slider_parent_class)->constructed (object);
  }
}

static void
dalston_volume_slider_class_init (DalstonVolumeSliderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DalstonVolumeSliderPrivate));

  object_class->get_property = dalston_volume_slider_get_property;
  object_class->set_property = dalston_volume_slider_set_property;
  object_class->dispose = dalston_volume_slider_dispose;
  object_class->finalize = dalston_volume_slider_finalize;
  object_class->constructed = dalston_volume_slider_constructed;

  pspec = g_param_spec_object ("sink",
                               "Sink.",
                               "The sink to use.",
                               GVC_TYPE_MIXER_STREAM,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SINK, pspec);
}

static void
_range_value_changed_cb (DalstonVolumeSlider *slider)
{
  DalstonVolumeSliderPrivate *priv = GET_PRIVATE (slider);
  guint volume;

  g_signal_handlers_block_by_func (priv->sink,
                                   _stream_volume_notify_cb,
                                   slider);
  volume = (float)gtk_range_get_value (GTK_RANGE (slider)) / 100.0 * PA_VOLUME_NORM;
  gvc_mixer_stream_change_volume (priv->sink, volume);
  g_signal_handlers_unblock_by_func (priv->sink,
                                     _stream_volume_notify_cb,
                                     slider);
}

static gchar *
_scale_format_value_cb (GtkScale *slider,
                        gdouble   value,
                        gpointer  userdata)
{
    gchar *format = NULL;

    if (value == 100.0)
      format = g_strdup (_("Loudest"));
    else if (value >= 75.0 && value < 100.0)
      format = g_strdup (_("Loud"));
    else if (value >= 60.0 && value < 75.0)
      format = g_strdup (_("Fairly loud"));
    else if (value >= 45.0 && value < 60.0)
      format = g_strdup (_("Average"));
    else if (value >= 25.0 && value < 45.0)
      format = g_strdup (_("Fairly quiet"));
    else if (value > 0.0 && value < 25.0)
      format = g_strdup (_("Quiet"));
    else
      format = g_strdup (_("Silent"));

    return format;
}


static void
dalston_volume_slider_init (DalstonVolumeSlider *self)
{
  g_signal_connect (self,
                    "format-value",
                    (GCallback)_scale_format_value_cb,
                    self);
  g_signal_connect (self,
                    "value-changed",
                    (GCallback)_range_value_changed_cb,
                    self);
  gtk_scale_set_value_pos (GTK_SCALE (self),
                           GTK_POS_BOTTOM);

}

GtkWidget *
dalston_volume_slider_new (void)
{
  return g_object_new (DALSTON_TYPE_VOLUME_SLIDER, NULL);
}

static void
dalston_volume_slider_update (DalstonVolumeSlider *slider)
{
  DalstonVolumeSliderPrivate *priv = GET_PRIVATE (slider);
  guint volume;

  /* block emissions of the value-changed on the slider */
  g_signal_handlers_block_by_func (slider,
                                   _range_value_changed_cb,
                                   slider);
  volume = 100 * 
    ((float)gvc_mixer_stream_get_volume (priv->sink) / PA_VOLUME_NORM);
  gtk_range_set_value (GTK_RANGE (slider),
                       (gdouble)volume);
  gtk_range_set_fill_level (GTK_RANGE (slider),
                            (gdouble)volume);
  g_signal_handlers_unblock_by_func (slider,
                                     _range_value_changed_cb,
                                     slider);
}

static void
_stream_volume_notify_cb (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    userdata)
{
  DalstonVolumeSlider *slider = (DalstonVolumeSlider *)userdata;

  dalston_volume_slider_update (slider);
}

static void
dalston_volume_slider_update_sink (DalstonVolumeSlider *slider,
                                   GvcMixerStream      *new_sink)
{
  DalstonVolumeSliderPrivate *priv = GET_PRIVATE (slider);

  if (priv->sink)
  {
    g_signal_handlers_disconnect_by_func (priv->sink,
                                          _stream_volume_notify_cb,
                                          slider);
    g_object_unref (priv->sink);
    priv->sink = NULL;
  }

  if (new_sink)
  {
    priv->sink = g_object_ref (new_sink);

    g_signal_connect (priv->sink,
                      "notify::volume",
                      (GCallback)_stream_volume_notify_cb,
                      slider);
    dalston_volume_slider_update (slider);
  }
}
