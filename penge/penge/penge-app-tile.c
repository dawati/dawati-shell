#include <gtk/gtk.h>

#include "penge-app-tile.h"
#include "penge-app-bookmark-manager.h"

G_DEFINE_TYPE (PengeAppTile, penge_app_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APP_TILE, PengeAppTilePrivate))

typedef struct _PengeAppTilePrivate PengeAppTilePrivate;

struct _PengeAppTilePrivate {
  PengeAppBookmark *bookmark;
  ClutterActor *tex;
};

enum
{
  PROP_0,
  PROP_BOOKMARK
};

#define ICON_SIZE 48

static void
penge_app_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);


  switch (property_id) {
    case PROP_BOOKMARK:
      g_value_set_boxed (value, priv->bookmark);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_app_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_BOOKMARK:
      priv->bookmark = g_value_dup_boxed (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_app_tile_dispose (GObject *object)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);

  if (priv->bookmark)
  {
    penge_app_bookmark_unref (priv->bookmark);
    priv->bookmark = NULL;
  }

  G_OBJECT_CLASS (penge_app_tile_parent_class)->dispose (object);
}

static void
penge_app_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_app_tile_parent_class)->finalize (object);
}

static void
penge_app_tile_constructed (GObject *object)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);
  GtkIconTheme *icon_theme;
  GtkIconInfo *info;
  const gchar *path;
  GError *error = NULL;

  g_return_if_fail (priv->bookmark);

  if (!priv->bookmark)
    return;

  if (!priv->bookmark->icon_name)
    return;

  icon_theme = gtk_icon_theme_new ();
  info = gtk_icon_theme_lookup_icon (icon_theme,
                                     priv->bookmark->icon_name,
                                     ICON_SIZE,
                                     0); /* no flags */
  path = gtk_icon_info_get_filename (info);

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->tex),
                                      path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error loading texture from file: %s",
               error->message);
    g_clear_error (&error);
  }
}

static void
penge_app_tile_class_init (PengeAppTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeAppTilePrivate));

  object_class->get_property = penge_app_tile_get_property;
  object_class->set_property = penge_app_tile_set_property;
  object_class->dispose = penge_app_tile_dispose;
  object_class->finalize = penge_app_tile_finalize;
  object_class->constructed = penge_app_tile_constructed;

  pspec = g_param_spec_boxed ("bookmark",
                              "bookmark",
                              "bookmark",
                              PENGE_TYPE_APP_BOOKMARK,
                              G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_BOOKMARK, pspec);
}

static void
penge_app_tile_init (PengeAppTile *self)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (self);

  priv->tex = clutter_texture_new ();
  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->tex,
                        0,
                        0);

}

