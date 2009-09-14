/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <moblin-panel/mpl-utils.h>

#include "penge-interesting-tile.h"

#include "penge-utils.h"
#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeInterestingTile, penge_interesting_tile, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_INTERESTING_TILE, PengeInterestingTilePrivate))

typedef struct _PengeInterestingTilePrivate PengeInterestingTilePrivate;

struct _PengeInterestingTilePrivate {
  NbtkWidget *inner_table;

  ClutterActor *body;
  ClutterActor *icon;
  NbtkWidget *bin;
  NbtkWidget *primary_text;
  NbtkWidget *secondary_text;
  NbtkWidget *details_overlay;
  NbtkWidget *remove_button;
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

guint signals [LAST_SIGNAL];

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
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->bin),
                                        priv->body);
      }

      priv->body = g_value_get_object (value);

      if (!priv->body)
        return;

      nbtk_bin_set_child (NBTK_BIN (priv->bin),
                          priv->body);


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
        clutter_actor_show (priv->icon);
      else
        clutter_actor_hide (priv->icon);

      break;
    case PROP_PRIMARY_TEXT:
      nbtk_label_set_text (NBTK_LABEL (priv->primary_text),
                           g_value_get_string (value));
      break;
    case PROP_SECONDARY_TEXT:
      nbtk_label_set_text (NBTK_LABEL (priv->secondary_text),
                           g_value_get_string (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_interesting_tile_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_interesting_tile_parent_class)->dispose (object);
}

static void
penge_interesting_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_interesting_tile_parent_class)->finalize (object);
}

static void
penge_interesting_tile_class_init (PengeInterestingTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeInterestingTilePrivate));

  object_class->get_property = penge_interesting_tile_get_property;
  object_class->set_property = penge_interesting_tile_set_property;
  object_class->dispose = penge_interesting_tile_dispose;
  object_class->finalize = penge_interesting_tile_finalize;

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

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (userdata);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  if (1)
  {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0xc0,
                           NULL);
    clutter_actor_animate (priv->remove_button,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0xff, /* asset already has opacity */
                           NULL);
  } else {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0,
                           NULL);
  }

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (userdata);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  if (0)
  {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0xc0,
                           NULL);
  } else {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0,
                           NULL);
    clutter_actor_animate (priv->remove_button,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0,
                           NULL);
  }

  return FALSE;
}

static void
_remove_button_clicked (NbtkButton *button,
                        gpointer    userdata)
{
  PengeInterestingTile *tile = PENGE_INTERESTING_TILE (userdata);

  g_signal_emit (tile, signals[REMOVE_CLICKED_SIGNAL], 0);
}

static void
penge_interesting_tile_init (PengeInterestingTile *self)
{
  PengeInterestingTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;
  ClutterAlpha *alpha;
  NbtkWidget *icon;

  priv->inner_table = nbtk_table_new ();
  nbtk_bin_set_child (NBTK_BIN (self),
                      (ClutterActor *)priv->inner_table);
  nbtk_bin_set_fill (NBTK_BIN (self), TRUE, TRUE);
  priv->bin = nbtk_bin_new ();
  nbtk_bin_set_fill (NBTK_BIN (priv->bin), TRUE, TRUE);
  nbtk_table_add_actor (NBTK_TABLE (priv->inner_table),
                        (ClutterActor *)priv->bin,
                        0,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               (ClutterActor *)priv->bin,
                               "y-align", 0.0,
                               "x-align", 0.0,
                               "row-span", 2,
                               "y-fill", TRUE,
                               "y-expand", TRUE,
                               NULL);

  priv->primary_text = nbtk_label_new ("Primary text");
  nbtk_widget_set_style_class_name (priv->primary_text, 
                                    "PengeInterestingTilePrimaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->primary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);

  priv->secondary_text = nbtk_label_new ("Secondary text");
  nbtk_widget_set_style_class_name (priv->secondary_text, 
                                    "PengeInterestingTileSecondaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->secondary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);

  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon, 28, 28);
  clutter_actor_hide (priv->icon);

  /* This gets added to ourself table after our body because of ordering */
  priv->details_overlay = nbtk_table_new ();
  nbtk_widget_set_style_class_name (priv->details_overlay,
                                    "PengeInterestingTileDetails");
  clutter_actor_set_opacity ((ClutterActor *)priv->details_overlay, 0x0);

  nbtk_table_add_actor (NBTK_TABLE (priv->inner_table),
                        (ClutterActor *)priv->details_overlay,
                        1,
                        0);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               (ClutterActor *)priv->details_overlay,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               TRUE,
                               "y-fill",
                               FALSE,
                               "y-align",
                               1.0,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        (ClutterActor *)priv->primary_text,
                        0,
                        1);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               (ClutterActor *)priv->primary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        (ClutterActor *)priv->secondary_text,
                        1,
                        1);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               (ClutterActor *)priv->secondary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  /*
   * Explicitly set the width to 100 to avoid overflowing text. Slightly
   * hackyish but works around a strange bug in NbtkTable where if the text
   * is too long it will cause negative positioning of the icon.
   */
  clutter_actor_set_width ((ClutterActor *)priv->primary_text, 100);
  clutter_actor_set_width ((ClutterActor *)priv->secondary_text, 100);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
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
                               "allocate-hidden", FALSE,
                               NULL);

  priv->remove_button = nbtk_button_new ();
  nbtk_widget_set_style_class_name (priv->remove_button,
                                    "PengeInterestingTileRemoveButton");
  icon = nbtk_icon_new ();
  nbtk_widget_set_style_class_name (icon,
                                    "PengeInterestingTileIcon");
  nbtk_bin_set_child (NBTK_BIN (priv->remove_button),
                      icon);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                        priv->remove_button,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 1.0,
                                        "y-align", 0.0,
                                        NULL);

  clutter_actor_set_opacity (priv->remove_button, 0x0);

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    self);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    self);

  g_signal_connect (priv->remove_button,
                    "clicked",
                    (GCallback)_remove_button_clicked,
                    self);

  nbtk_table_set_col_spacing (NBTK_TABLE (priv->details_overlay), 4);

  clutter_actor_set_reactive ((ClutterActor *) self, TRUE);
}
