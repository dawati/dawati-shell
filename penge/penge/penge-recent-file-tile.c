#include <gtk/gtk.h>

#include "penge-recent-file-tile.h"
#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeRecentFileTile, penge_recent_file_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTilePrivate))

typedef struct _PengeRecentFileTilePrivate PengeRecentFileTilePrivate;

struct _PengeRecentFileTilePrivate {
    gchar *thumbnail_path;
    GtkRecentInfo *info;
};

enum
{
  PROP_0,
  PROP_THUMBNAIL_PATH,
  PROP_INFO
};

static void
penge_recent_file_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_THUMBNAIL_PATH:
      g_value_set_string (value, priv->thumbnail_path);
      break;
    case PROP_INFO:
      g_value_set_pointer (value, priv->info);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_THUMBNAIL_PATH:
      priv->thumbnail_path = g_value_dup_string (value);
      break;
    case PROP_INFO:
      priv->info = (GtkRecentInfo *)g_value_get_pointer (value);
      gtk_recent_info_ref (priv->info);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_dispose (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  if (priv->info)
  {
    gtk_recent_info_unref (priv->info);
    priv->info = NULL;
  }

  G_OBJECT_CLASS (penge_recent_file_tile_parent_class)->dispose (object);
}

static void
penge_recent_file_tile_finalize (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  g_free (priv->thumbnail_path);

  G_OBJECT_CLASS (penge_recent_file_tile_parent_class)->finalize (object);
}

static void
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (userdata);
  gchar *app_exec = NULL;
  gchar *last_application;
  GError *error = NULL;

  last_application = gtk_recent_info_last_application (priv->info);

  if (!gtk_recent_info_get_application_info (priv->info,
                                             last_application,
                                             &app_exec,
                                             NULL,
                                             NULL))
  {
    g_warning (G_STRLOC ": Error getting application command line: %s",
               error->message);
    g_clear_error (&error);
  } else {
    if (!g_spawn_command_line_async (app_exec, &error))
    {
      g_warning (G_STRLOC ": Error launching: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      penge_utils_signal_activated (actor);
    }
  }

  g_free (last_application);
}

static void
penge_recent_file_tile_constructed (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);
  ClutterActor *tex;
  GError *error = NULL;

  tex = g_object_new (PENGE_TYPE_MAGIC_TEXTURE,
                      NULL);

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (tex),
                                      priv->thumbnail_path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error opening thumbnail: %s",
               error->message);
    g_clear_error (&error);
  } else {
    nbtk_table_add_actor (NBTK_TABLE (object),
                          tex,
                          0,
                          0);
  }

  g_signal_connect (object, 
                    "button-press-event",
                    (GCallback)_button_press_event,
                    object);
}

static void
penge_recent_file_tile_class_init (PengeRecentFileTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeRecentFileTilePrivate));

  object_class->get_property = penge_recent_file_tile_get_property;
  object_class->set_property = penge_recent_file_tile_set_property;
  object_class->dispose = penge_recent_file_tile_dispose;
  object_class->finalize = penge_recent_file_tile_finalize;
  object_class->constructed = penge_recent_file_tile_constructed;

  pspec = g_param_spec_string ("thumbnail-path",
                               "Thumbnail path",
                               "Path to the thumbnail to use to represent"
                               "this recent file",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_THUMBNAIL_PATH, pspec);

  pspec = g_param_spec_pointer ("info",
                                "Recent file information",
                                "The GtkRecentInfo structure for this recent"
                                "file",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_INFO, pspec);
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");
  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);
  return FALSE;
}

static void
penge_recent_file_tile_init (PengeRecentFileTile *self)
{
  NbtkPadding padding = { CLUTTER_UNITS_FROM_DEVICE (4),
                          CLUTTER_UNITS_FROM_DEVICE (4),
                          CLUTTER_UNITS_FROM_DEVICE (4),
                          CLUTTER_UNITS_FROM_DEVICE (4) };
  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    NULL);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    NULL);
  nbtk_widget_set_padding (NBTK_WIDGET (self), &padding);
}


const gchar *
penge_recent_file_tile_get_uri (PengeRecentFileTile *tile)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (tile);

  return gtk_recent_info_get_uri (priv->info);
}

