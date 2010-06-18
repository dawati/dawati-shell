/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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

#include <meego-panel/mpl-utils.h>

#include "penge-interesting-tile.h"

#include "penge-utils.h"
#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeInterestingTile, penge_interesting_tile, MX_TYPE_BUTTON)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_INTERESTING_TILE, PengeInterestingTilePrivate))

#define GET_PRIVATE(o) ((PengeInterestingTile *)o)->priv

struct _PengeInterestingTilePrivate {
  ClutterActor *inner_table;

  ClutterActor *body;
  ClutterActor *icon;
  ClutterActor *primary_text;
  ClutterActor *secondary_text;
  ClutterActor *details_overlay;
  ClutterActor *remove_button;

  guint tooltip_idle_id;
};

enum
{
  PROP_0,
  PROP_BODY,
  PROP_ICON_PATH,
  PROP_PRIMARY_TEXT,
  PROP_SECONDARY_TEXT,
};

enum
{
  REMOVE_CLICKED_SIGNAL,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL];

static void
penge_interesting_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_interesting_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (object);
  GError *error = NULL;
  const gchar *path;

  switch (property_id) {
    case PROP_BODY:
      if (priv->body)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->inner_table),
                                        priv->body);
      }

      priv->body = g_value_get_object (value);

      if (!priv->body)
        return;

      mx_table_add_actor_with_properties (MX_TABLE (priv->inner_table),
                                          priv->body,
                                          0, 0,
                                          "y-align", MX_ALIGN_START,
                                          "x-align", MX_ALIGN_START,
                                          "y-fill", TRUE,
                                          "y-expand", TRUE,
                                          NULL);
      break;
    case PROP_ICON_PATH:
      path = g_value_get_string (value);

      if (path && !clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon), 
                                          path,
                                          &error))
      {
        g_critical (G_STRLOC ": error setting icon texture from file: %s",
                    error->message);
        g_clear_error (&error);
      }

      if (path)
      {
        clutter_actor_show (priv->icon);
        clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                                     priv->primary_text,
                                     "column", 1,
                                     NULL);
        clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                                     priv->secondary_text,
                                     "column", 1,
                                     NULL);
      } else {
        clutter_actor_hide (priv->icon);
        clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                                     priv->primary_text,
                                     "column", 0,
                                     NULL);
        clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                                     priv->secondary_text,
                                     "column", 0,
                                     NULL);
      }

      break;
    case PROP_PRIMARY_TEXT:
      mx_label_set_text (MX_LABEL (priv->primary_text),
                         g_value_get_string (value));
      break;
    case PROP_SECONDARY_TEXT:
      mx_label_set_text (MX_LABEL (priv->secondary_text),
                         g_value_get_string (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_interesting_tile_dispose (GObject *object)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (object);

  if (priv->tooltip_idle_id)
  {
    g_source_remove (priv->tooltip_idle_id);
    priv->tooltip_idle_id = 0;
  }

  G_OBJECT_CLASS (penge_interesting_tile_parent_class)->dispose (object);
}

static void
penge_interesting_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_interesting_tile_parent_class)->finalize (object);
}

static gboolean
penge_interesting_tile_enter_event (ClutterActor         *actor,
                                    ClutterCrossingEvent *event)
{
  /* If we are just entering from a child then don't set the hover */
  if (event->related &&
      clutter_actor_get_parent (event->related) == actor)
  {
    return FALSE;
  }

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor), "hover");

  return FALSE;
}

static gboolean
penge_interesting_tile_leave_event (ClutterActor         *actor,
                                    ClutterCrossingEvent *event)
{
  /* If we are just leaving to a child then don't unset the hover */
  if (event->related &&
      clutter_actor_get_parent (event->related) == actor)
  {
    return FALSE;
  }

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor), "");

  return FALSE;
}

static void
penge_interesting_tile_class_init (PengeInterestingTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeInterestingTilePrivate));

  object_class->get_property = penge_interesting_tile_get_property;
  object_class->set_property = penge_interesting_tile_set_property;
  object_class->dispose = penge_interesting_tile_dispose;
  object_class->finalize = penge_interesting_tile_finalize;

  actor_class->enter_event = penge_interesting_tile_enter_event;
  actor_class->leave_event = penge_interesting_tile_leave_event;

  pspec = g_param_spec_object ("body",
                               "Body",
                               "Actor for the body of the tile",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_BODY, pspec);

  pspec = g_param_spec_string ("icon-path",
                               "Icon path",
                               "Path to icon to use in the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_ICON_PATH, pspec);

  pspec = g_param_spec_string ("primary-text",
                               "Primary text",
                               "Primary text for the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_PRIMARY_TEXT, pspec);

  pspec = g_param_spec_string ("secondary-text",
                               "Secondary text",
                               "Secondary text for the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_SECONDARY_TEXT, pspec);

  signals[REMOVE_CLICKED_SIGNAL] = 
    g_signal_new ("remove-clicked",
                  PENGE_TYPE_INTERESTING_TILE,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
_remove_button_clicked (MxButton *button,
                        gpointer  userdata)
{
  PengeInterestingTile *tile = PENGE_INTERESTING_TILE (userdata);

  g_signal_emit (tile, signals[REMOVE_CLICKED_SIGNAL], 0);
}


static gboolean
_labels_update_tooltip_idle_cb (PengeInterestingTile *tile)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (tile);
  ClutterActor *tmp_text;
  PangoLayout *layout;

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->primary_text));
  layout = clutter_text_get_layout (CLUTTER_TEXT (tmp_text));
  if (pango_layout_is_ellipsized (layout))
    mx_widget_set_tooltip_text (MX_WIDGET (priv->primary_text),
                                mx_label_get_text (MX_LABEL (priv->primary_text)));
  else
    mx_widget_set_tooltip_text (MX_WIDGET (priv->primary_text),
                                NULL);

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->secondary_text));
  layout = clutter_text_get_layout (CLUTTER_TEXT (tmp_text));
  if (pango_layout_is_ellipsized (layout))
    mx_widget_set_tooltip_text (MX_WIDGET (priv->secondary_text),
                                mx_label_get_text (MX_LABEL (priv->secondary_text)));
  else
    mx_widget_set_tooltip_text (MX_WIDGET (priv->secondary_text),
                                NULL);

  priv->tooltip_idle_id = 0;


  return FALSE;
}

static void
_label_notify_allocation_cb (MxLabel              *label,
                             GParamSpec           *pspec,
                             PengeInterestingTile *tile)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (tile);

  if (!priv->tooltip_idle_id)
    priv->tooltip_idle_id = g_idle_add ((GSourceFunc)_labels_update_tooltip_idle_cb,
                                        tile);
}

static void
penge_interesting_tile_init (PengeInterestingTile *self)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE_REAL (self);
  ClutterActor *tmp_text;
  ClutterActor *icon;

  self->priv = priv;

  priv->inner_table = mx_table_new ();
  mx_bin_set_child (MX_BIN (self),
                    priv->inner_table);
  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);

  priv->primary_text = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->primary_text), 
                               "PengeInterestingTilePrimaryLabel");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->primary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);
  g_signal_connect (priv->primary_text,
                    "notify::allocation",
                    (GCallback)_label_notify_allocation_cb,
                    self);

  priv->secondary_text = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->secondary_text), 
                               "PengeInterestingTileSecondaryLabel");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->secondary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);
  g_signal_connect (priv->secondary_text,
                    "notify::allocation",
                    (GCallback)_label_notify_allocation_cb,
                    self);

  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon, 28, 28);
  clutter_actor_hide (priv->icon);

  /* This gets added to ourself table after our body because of ordering */
  priv->details_overlay = mx_table_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->details_overlay),
                               "PengeInterestingTileDetails");

  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->details_overlay,
                      1,
                      0);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               (ClutterActor *)priv->details_overlay,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
                               "y-fill", FALSE,
                               "y-align", MX_ALIGN_END,
                               NULL);

  mx_table_add_actor (MX_TABLE (priv->details_overlay),
                      priv->primary_text,
                      0,
                      1);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               (ClutterActor *)priv->primary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  mx_table_add_actor (MX_TABLE (priv->details_overlay),
                      priv->secondary_text,
                      1,
                      1);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               (ClutterActor *)priv->secondary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  mx_table_add_actor (MX_TABLE (priv->details_overlay),
                      priv->icon,
                      0,
                      0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               priv->icon,
                               "row-span", 2,
                               "y-expand", FALSE,
                               "x-expand", FALSE,
                               "y-fill", FALSE,
                               "x-fill", FALSE,
                               NULL);

  priv->remove_button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->remove_button),
                               "PengeInterestingTileRemoveButton");
  icon = (ClutterActor *)mx_icon_new ();
  mx_stylable_set_style_class (MX_STYLABLE (icon),
                               "PengeInterestingTileIcon");
  mx_bin_set_child (MX_BIN (priv->remove_button),
                      (ClutterActor *)icon);
  mx_table_add_actor_with_properties (MX_TABLE (priv->details_overlay),
                                      priv->remove_button,
                                      0, 2,
                                      "row-span", 2,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_END,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);

  g_signal_connect (priv->remove_button,
                    "clicked",
                    (GCallback)_remove_button_clicked,
                    self);

  mx_table_set_column_spacing (MX_TABLE (priv->details_overlay), 4);

  clutter_actor_set_reactive ((ClutterActor *) self, TRUE);

  clutter_actor_hide (priv->icon);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               priv->primary_text,
                               "column", 0,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               priv->secondary_text,
                               "column", 0,
                               NULL);
}
