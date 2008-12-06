#include "penge-recent-file-tile.h"

G_DEFINE_TYPE (PengeRecentFileTile, penge_recent_file_tile, PENGE_TYPE_TILE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTilePrivate))

typedef struct _PengeRecentFileTilePrivate PengeRecentFileTilePrivate;

struct _PengeRecentFileTilePrivate {
    gchar *uri;
    ClutterActor *thumbnail;
};

enum
{
  PROP_0,
  PROP_URI
};

static void penge_recent_file_tile_update_thumbnail (PengeRecentFileTile *tile);

static void
penge_recent_file_tile_get_property (GObject    *object, 
                                     guint       property_id,
                                     GValue     *value, 
                                     GParamSpec *pspec)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value, 
                                     GParamSpec   *pspec)
{
  PengeRecentFileTile *tile = PENGE_RECENT_FILE_TILE (object);
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_URI:
      g_free (priv->uri);
      priv->uri = g_value_dup_string (value);
      penge_recent_file_tile_update_thumbnail (tile);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_dispose (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  if (priv->thumbnail)
  {
    g_object_unref (priv->thumbnail);
    priv->thumbnail = NULL;
  }

  G_OBJECT_CLASS (penge_recent_file_tile_parent_class)->dispose (object);
}

static void
penge_recent_file_tile_finalize (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  g_free (priv->uri);

  G_OBJECT_CLASS (penge_recent_file_tile_parent_class)->finalize (object);
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

  pspec = g_param_spec_string ("uri",
                               "uri",
                               "URI to file",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_URI, pspec);
}

static void
penge_recent_file_tile_init (PengeRecentFileTile *self)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (self);

  priv->thumbnail = clutter_texture_new ();
  g_object_set (self, "child", priv->thumbnail, NULL);
}

static void
penge_recent_file_tile_update_thumbnail (PengeRecentFileTile *tile)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (tile);
  GError *error = NULL;
  gchar *thumbnail_path;
  gchar *thumbnail_filename;
  gchar *csum;

  g_assert (priv->uri);

  csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, priv->uri, -1);
  thumbnail_filename = g_strconcat (csum, ".png", NULL);
  thumbnail_path = g_build_filename (g_get_home_dir (),
                                     ".thumbnails",
                                     "normal",
                                     thumbnail_filename,
                                     NULL);

  g_debug (G_STRLOC ": Using thumbnail path: %s", thumbnail_path);
  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->thumbnail),
                                      thumbnail_path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error loading thumbnail: %s", error->message);
    g_clear_error (&error);
  }

  g_free (csum);
  g_free (thumbnail_filename);
  g_free (thumbnail_path);
}
