#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "penge-app-tile.h"
#include "penge-app-bookmark-manager.h"
#include "penge-utils.h"

G_DEFINE_TYPE (PengeAppTile, penge_app_tile, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APP_TILE, PengeAppTilePrivate))

typedef struct _PengeAppTilePrivate PengeAppTilePrivate;

struct _PengeAppTilePrivate {
  PengeAppBookmark *bookmark;
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
  GAppInfo *app_info;
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

  if (priv->app_info)
  {
    g_object_unref (priv->app_info);
    priv->app_info = NULL;
  }

  G_OBJECT_CLASS (penge_app_tile_parent_class)->dispose (object);
}

static void
penge_app_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_app_tile_parent_class)->finalize (object);
}

static void
_update_icon_from_icon_theme (PengeAppTile *tile)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (tile);
  const gchar *path;
  GError *error = NULL;
  GtkIconInfo *info;
  GIcon *icon;

  icon = g_app_info_get_icon (priv->app_info);
  info = gtk_icon_theme_lookup_by_gicon (priv->icon_theme,
                                         icon,
                                         ICON_SIZE,
                                         GTK_ICON_LOOKUP_GENERIC_FALLBACK);

  if (!info)
  {
    /* try falling back */
    info = gtk_icon_theme_lookup_icon (priv->icon_theme,
                                       "applications-other",
                                       ICON_SIZE,
                                       GTK_ICON_LOOKUP_GENERIC_FALLBACK);
  }

  if (info)
  {
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
}

void
_icon_theme_changed_cb (GtkIconTheme *icon_theme,
                        gpointer      userdata)
{
  PengeAppTile *tile = (PengeAppTile *)userdata;

  _update_icon_from_icon_theme (tile);
}

static void
penge_app_tile_constructed (GObject *object)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);
  gchar *path;
  GError *error = NULL;

  g_return_if_fail (priv->bookmark);

  if (!priv->bookmark)
    return;

  priv->icon_theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->icon_theme,
                    "changed",
                    (GCallback)_icon_theme_changed_cb,
                    object);

  path = g_filename_from_uri (priv->bookmark->uri, NULL, &error);

  if (path)
  {
    priv->app_info = G_APP_INFO (g_desktop_app_info_new_from_filename (path));
    nbtk_button_set_tooltip (NBTK_BUTTON (object),
                             g_app_info_get_name (priv->app_info));
    g_free (path);
  }

  if (error)
  {
    g_warning (G_STRLOC ": Error getting info from bookmark: %s",
               error->message);
    g_clear_error (&error);
  }

  _update_icon_from_icon_theme ((PengeAppTile *)object);
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

static gboolean
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (userdata);
  GError *error = NULL;
  GAppLaunchContext *context;

  context = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());

  if (g_app_info_launch (priv->app_info, NULL, context, &error))
    penge_utils_signal_activated (actor);

  if (error)
  {
    g_warning (G_STRLOC ": Error launching application): %s",
                 error->message);
    g_clear_error (&error);
  }

  g_object_unref (context);
  return FALSE;
}

static void
penge_app_tile_init (PengeAppTile *self)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (self);

  priv->tex = clutter_texture_new ();

  nbtk_bin_set_child (NBTK_BIN (self),
                      priv->tex);
  g_signal_connect (self,
                    "button-press-event",
                    (GCallback)_button_press_event,
                    self);
}

