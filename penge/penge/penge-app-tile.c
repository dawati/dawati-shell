#include <gtk/gtk.h>
#include <gio/gio.h>

#include "penge-app-tile.h"
#include "penge-app-bookmark-manager.h"
#include "penge-utils.h"

G_DEFINE_TYPE (PengeAppTile, penge_app_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APP_TILE, PengeAppTilePrivate))

typedef struct _PengeAppTilePrivate PengeAppTilePrivate;

struct _PengeAppTilePrivate {
  PengeAppBookmark *bookmark;
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
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
_update_icon_from_icon_theme (PengeAppTile *tile)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (tile);
  const gchar *path;
  GError *error = NULL;
  GtkIconInfo *info;

  info = gtk_icon_theme_lookup_icon (priv->icon_theme,
                                     priv->bookmark->icon_name,
                                     ICON_SIZE,
                                     GTK_ICON_LOOKUP_GENERIC_FALLBACK);
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

  g_return_if_fail (priv->bookmark);

  if (!priv->bookmark)
    return;

  if (!priv->bookmark->icon_name)
    return;

  priv->icon_theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->icon_theme,
                    "changed",
                    (GCallback)_icon_theme_changed_cb,
                    object);

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

static gboolean
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (userdata);
  GError *error = NULL;
  gchar **argv = NULL;

  if (!(priv->bookmark && priv->bookmark->app_exec))
    return TRUE;

  argv = g_strsplit (priv->bookmark->app_exec, " ", 0);

  if (!g_spawn_async  (NULL,
                       argv,
                       NULL,
                       G_SPAWN_SEARCH_PATH,
                       NULL,
                       NULL,
                       NULL,
                       &error))
  {
    g_warning (G_STRLOC ": Error launching application): %s",
               error->message);
    g_clear_error (&error);
  } else {
    penge_utils_signal_activated (actor);
  }

  g_strfreev (argv);

  return TRUE;
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

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    self);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    self);
  g_signal_connect (self,
                    "button-press-event",
                    (GCallback)_button_press_event,
                    self);
}

