#include "dalston-volume-status-icon.h"
#include <libgvc/gvc-mixer-stream.h>

G_DEFINE_TYPE (DalstonVolumeStatusIcon, dalston_volume_status_icon, GTK_TYPE_STATUS_ICON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_STATUS_ICON, DalstonVolumeStatusIconPrivate))

typedef struct _DalstonVolumeStatusIconPrivate DalstonVolumeStatusIconPrivate;

struct _DalstonVolumeStatusIconPrivate {
  GvcMixerStream *sink;

  gboolean active;
};

enum
{
  PROP_0,
  PROP_SINK
};

static void dalston_volume_status_icon_update_sink (DalstonVolumeStatusIcon *icon,
                                                    GvcMixerStream           *new_sink);


typedef enum
{
  VOLUME_STATUS_ICON_MUTED,
  VOLUME_STATUS_ICON_MUTED_ACTIVE,
  VOLUME_STATUS_ICON_LOW,
  VOLUME_STATUS_ICON_LOW_ACTIVE,
  VOLUME_STATUS_ICON_MEDIUM,
  VOLUME_STATUS_ICON_MEDIUM_ACTIVE,
  VOLUME_STATUS_ICON_HIGH,
  VOLUME_STATUS_ICON_HIGH_ACTIVE,
} VolumeIconState;

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static const gchar *icon_names[] = {
  PKG_ICON_DIR "/" "dalston-volume-applet-0-normal.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-0-active.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-1-normal.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-1-active.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-2-normal.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-2-active.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-3-normal.png",
  PKG_ICON_DIR "/" "dalston-volume-applet-3-active.png"
};

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
  VolumeIconState icon_state;

  if (gvc_mixer_stream_get_is_muted (priv->sink))
  {
    icon_state = VOLUME_STATUS_ICON_MUTED;
  } else {
    volume = 100 * 
      ((float)gvc_mixer_stream_get_volume (priv->sink) / PA_VOLUME_NORM);
    if (volume < 45)
    {
      icon_state = VOLUME_STATUS_ICON_LOW;
    } else if (volume < 75) {
      icon_state = VOLUME_STATUS_ICON_MEDIUM;
    } else {
      icon_state = VOLUME_STATUS_ICON_HIGH;
    }
  }

  if (priv->active)
    icon_state++;

  gtk_status_icon_set_from_file (GTK_STATUS_ICON (icon),
                                 icon_names[icon_state]);
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

void
dalston_volume_status_icon_set_active (DalstonVolumeStatusIcon *icon,
                                       gboolean                 active)
{
  DalstonVolumeStatusIconPrivate *priv = GET_PRIVATE (icon);

  priv->active = active;
  dalston_volume_status_icon_update (icon);
}
