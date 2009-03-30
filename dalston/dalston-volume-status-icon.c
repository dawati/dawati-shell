#include "dalston-volume-status-icon.h"
#include <libgvc/gvc-mixer-stream.h>

G_DEFINE_TYPE (DalstonVolumeStatusIcon, dalston_volume_status_icon, GTK_TYPE_STATUS_ICON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_STATUS_ICON, DalstonVolumeStatusIconPrivate))

typedef struct _DalstonVolumeStatusIconPrivate DalstonVolumeStatusIconPrivate;

struct _DalstonVolumeStatusIconPrivate {
  GvcMixerStream *sink;
};

enum
{
  PROP_0,
  PROP_SINK
};

static void dalston_volume_status_icon_update_sink (DalstonVolumeStatusIcon *icon,
                                                    GvcMixerStream           *new_sink);

#define VOLUME_STATUS_ICON_MUTED  "audio-volume-muted"
#define VOLUME_STATUS_ICON_LOW    "audio-volume-low"
#define VOLUME_STATUS_ICON_MEDIUM "audio-volume-medium"
#define VOLUME_STATUS_ICON_HIGH   "audio-volume-high"

static void
dalston_volume_status_icon_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DalstonVolumeStatusIconPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SINK:
      g_value_set_object (value, priv->sink);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_status_icon_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DalstonVolumeStatusIcon *icon = (DalstonVolumeStatusIcon *)object;
  GvcMixerStream *sink;

  switch (property_id) {
    case PROP_SINK:
      sink = (GvcMixerStream *)g_value_get_object (value);
      dalston_volume_status_icon_update_sink (icon, sink);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_status_icon_dispose (GObject *object)
{
  dalston_volume_status_icon_update_sink ((DalstonVolumeStatusIcon *)object,
                                          NULL);

  G_OBJECT_CLASS (dalston_volume_status_icon_parent_class)->dispose (object);
}

static void
dalston_volume_status_icon_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_volume_status_icon_parent_class)->finalize (object);
}

static void
dalston_volume_status_icon_class_init (DalstonVolumeStatusIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DalstonVolumeStatusIconPrivate));

  object_class->get_property = dalston_volume_status_icon_get_property;
  object_class->set_property = dalston_volume_status_icon_set_property;
  object_class->dispose = dalston_volume_status_icon_dispose;
  object_class->finalize = dalston_volume_status_icon_finalize;

  pspec = g_param_spec_object ("sink",
                               "Sink.",
                               "The sink to use.",
                               GVC_TYPE_MIXER_STREAM,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SINK, pspec);
}

static void
dalston_volume_status_icon_init (DalstonVolumeStatusIcon *self)
{
}

GtkStatusIcon *
dalston_volume_status_icon_new (void)
{
  return g_object_new (DALSTON_TYPE_VOLUME_STATUS_ICON, NULL);
}

static void
dalston_volume_status_icon_update (DalstonVolumeStatusIcon *icon)
{
  DalstonVolumeStatusIconPrivate *priv = GET_PRIVATE (icon);
  guint volume;

  if (gvc_mixer_stream_get_is_muted (priv->sink))
  {
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (icon),
                                        VOLUME_STATUS_ICON_MUTED);
  } else {
    volume = 100 * 
      ((float)gvc_mixer_stream_get_volume (priv->sink) / PA_VOLUME_NORM);
    if (volume < 45)
    {
      gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (icon),
                                          VOLUME_STATUS_ICON_LOW);
    } else if (volume < 75) {
      gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (icon),
                                          VOLUME_STATUS_ICON_MEDIUM);
    } else {
      gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (icon),
                                          VOLUME_STATUS_ICON_HIGH);

    }
  }
}

static void
_stream_is_muted_notify_cb (GObject    *object,
                            GParamSpec *pspec,
                            gpointer    userdata)
{
  dalston_volume_status_icon_update ((DalstonVolumeStatusIcon *)userdata);
}

static void
_stream_volume_notify_cb (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    userdata)
{
  dalston_volume_status_icon_update ((DalstonVolumeStatusIcon *)userdata);
}

static void
dalston_volume_status_icon_update_sink (DalstonVolumeStatusIcon *icon,
                                        GvcMixerStream          *new_sink)
{
  DalstonVolumeStatusIconPrivate *priv = GET_PRIVATE (icon);

  if (priv->sink)
  {
    g_signal_handlers_disconnect_by_func (priv->sink,
                                          _stream_is_muted_notify_cb,
                                          icon);
    g_signal_handlers_disconnect_by_func (priv->sink,
                                          _stream_volume_notify_cb,
                                          icon);
    g_object_unref (priv->sink);
    priv->sink = NULL;
  }

  if (new_sink)
  {
    priv->sink = g_object_ref (new_sink);

    g_signal_connect (priv->sink,
                      "notify::is-muted",
                      (GCallback)_stream_is_muted_notify_cb,
                      icon);
    g_signal_connect (priv->sink,
                      "notify::volume",
                      (GCallback)_stream_volume_notify_cb,
                      icon);
    dalston_volume_status_icon_update (icon);
  }
}
