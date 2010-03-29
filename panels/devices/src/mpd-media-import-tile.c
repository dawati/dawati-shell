
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <glib/gi18n.h>

#include "mpd-gobject.h"
#include "mpd-media-import-tile.h"
#include "mpd-shell-defines.h"
#include "mpd-storage-device.h"
#include "mpd-text.h"

#include "config.h"

G_DEFINE_TYPE (MpdMediaImportTile, mpd_media_import_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_MEDIA_IMPORT_TILE, MpdMediaImportTilePrivate))

enum
{
  PROP_0,

  PROP_STORAGE_DEVICE
};

enum
{
  STOP_IMPORT,

  LAST_SIGNAL
};

typedef struct
{
  MpdStorageDevice  *storage;

  ClutterActor      *label;
  ClutterActor      *progress;

} MpdMediaImportTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_import_progress_cb (MpdStorageDevice   *storage,
                     float               progress,
                     MpdMediaImportTile *self)
{
  MpdMediaImportTilePrivate *priv = GET_PRIVATE (self);
  char *markup;

  progress = CLAMP (progress, 0, 100);

  markup = g_strdup_printf (_("<span font-weight='bold'>Importing data</span> "
                              "<span color='%s'>%d%%</span>"),
                            TEXT_COLOR,
                            (int) (progress * 100));
  clutter_text_set_markup (GET_CLUTTER_TEXT (priv->label), markup);
  g_free (markup);

  mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->progress), progress);
}

static void
_stop_clicked_cb (MxButton           *button,
                  MpdMediaImportTile *self)
{
  g_signal_emit_by_name (self, "stop-import");
}

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  MpdMediaImportTile *self = (MpdMediaImportTile *)
                              G_OBJECT_CLASS (mpd_media_import_tile_parent_class)
                                ->constructor (type, n_properties, properties);
  MpdMediaImportTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (priv->storage),
                        (GObject *) self);

  g_signal_connect (priv->storage, "progress",
                    G_CALLBACK (_import_progress_cb), self);

  return (GObject *) self;
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_STORAGE_DEVICE:
    g_value_set_object (value,
                        mpd_media_import_tile_get_storage_device (
                          MPD_MEDIA_IMPORT_TILE (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  MpdMediaImportTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
  {
  case PROP_STORAGE_DEVICE:
    /* Construct-only. */
    priv->storage = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdMediaImportTilePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->storage);

  G_OBJECT_CLASS (mpd_media_import_tile_parent_class)->dispose (object);
}

static void
mpd_media_import_tile_class_init (MpdMediaImportTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdMediaImportTilePrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_STORAGE_DEVICE,
                                   g_param_spec_object ("storage-device",
                                                        "Storage device",
                                                        "Storage device object",
                                                        MPD_TYPE_STORAGE_DEVICE,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /* Signals */

  _signals[STOP_IMPORT] = g_signal_new ("stop-import",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);
}

static void
mpd_media_import_tile_init (MpdMediaImportTile *self)
{
  MpdMediaImportTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *hbox;
  ClutterActor  *button;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  priv->label = mx_label_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->label);

  hbox = mx_box_layout_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), hbox);

  priv->progress = mx_progress_bar_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), priv->progress);

  button = mx_button_new_with_label (_("Stop"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_stop_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), priv->progress);
}

ClutterActor *
mpd_media_import_tile_new (MpdStorageDevice *storage)
{
  return g_object_new (MPD_TYPE_MEDIA_IMPORT_TILE,
                       "storage-device", storage,
                       NULL);
}

MpdStorageDevice *
mpd_media_import_tile_get_storage_device (MpdMediaImportTile *self)
{
  MpdMediaImportTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_MEDIA_IMPORT_TILE (self), NULL);

  return priv->storage;
}

