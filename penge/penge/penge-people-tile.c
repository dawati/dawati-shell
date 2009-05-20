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


#include "penge-people-tile.h"

G_DEFINE_TYPE (PengePeopleTile, penge_people_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_TILE, PengePeopleTilePrivate))

typedef struct _PengePeopleTilePrivate PengePeopleTilePrivate;

struct _PengePeopleTilePrivate {
    ClutterActor *body;
    ClutterActor *icon;
    NbtkWidget *primary_text;
    NbtkWidget *secondary_text;
    NbtkWidget *details_overlay;
    ClutterTimeline *timeline;
    ClutterBehaviour *behave;
};

enum
{
  PROP_0,
  PROP_BODY,
  PROP_ICON_PATH,
  PROP_PRIMARY_TEXT,
  PROP_SECONDARY_TEXT
};

#define DEFAULT_AVATAR_PATH PKG_DATADIR "/theme/mzone/default-avatar-icon.png"

static void
penge_people_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);
  GError *error = NULL;
  const gchar *path;

  switch (property_id) {
    case PROP_BODY:
      if (priv->body)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (object),
                                        priv->body);
        g_object_unref (priv->body);
      }

      priv->body = g_value_dup_object (value);
      nbtk_table_add_actor (NBTK_TABLE (object),
                            priv->body,
                            0,
                            0);
      clutter_container_child_set (CLUTTER_CONTAINER (object),
                                   priv->body,
                                   "y-align",
                                   0.0,
                                   "x-align",
                                   0.0,
                                   "row-span",
                                   2,
                                   "y-fill",
                                   TRUE,
                                   "y-expand",
                                   TRUE,
                                   NULL);

      nbtk_table_add_actor (NBTK_TABLE (object),
                            (ClutterActor *)priv->details_overlay,
                            1,
                            0);

      clutter_container_child_set (CLUTTER_CONTAINER (object),
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
      break;
    case PROP_ICON_PATH:

      path = g_value_get_string (value);

      if (!path)
      {
        path = DEFAULT_AVATAR_PATH;
      }

      if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon), 
                                          path,
                                          &error))
      {
        g_critical (G_STRLOC ": error setting icon texture from file: %s",
                    error->message);
        g_clear_error (&error);
      }

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
penge_people_tile_dispose (GObject *object)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);

  if (priv->timeline)
  {
    g_object_unref (priv->timeline);
    priv->timeline = NULL;
  }

  if (priv->behave)
  {
    g_object_unref (priv->behave);
    priv->behave = NULL;
  }

  G_OBJECT_CLASS (penge_people_tile_parent_class)->dispose (object);
}

static void
penge_people_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_tile_parent_class)->finalize (object);
}

static void
penge_people_tile_class_init (PengePeopleTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengePeopleTilePrivate));

  object_class->get_property = penge_people_tile_get_property;
  object_class->set_property = penge_people_tile_set_property;
  object_class->dispose = penge_people_tile_dispose;
  object_class->finalize = penge_people_tile_finalize;

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
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (userdata);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  clutter_timeline_set_direction (priv->timeline,
                                  CLUTTER_TIMELINE_FORWARD);
  if (!clutter_timeline_is_playing (priv->timeline))
  {
    clutter_timeline_rewind (priv->timeline);
    clutter_timeline_start (priv->timeline);
  }

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->primary_text),
                                      "hover");
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->secondary_text),
                                      "hover");

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (userdata);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->primary_text),
                                     NULL);
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->secondary_text),
                                      NULL);

  clutter_timeline_set_direction (priv->timeline,
                                  CLUTTER_TIMELINE_BACKWARD);
  if (!clutter_timeline_is_playing (priv->timeline))
    clutter_timeline_start (priv->timeline);



  return FALSE;
}

static void
penge_people_tile_init (PengePeopleTile *self)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;
  ClutterAlpha *alpha;

  priv->primary_text = nbtk_label_new ("Primary text");
  nbtk_widget_set_style_class_name (priv->primary_text, 
                                    "PengePeopleTilePrimaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->primary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);

  priv->secondary_text = nbtk_label_new ("Secondary text");
  nbtk_widget_set_style_class_name (priv->secondary_text, 
                                    "PengePeopleTileSecondaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->secondary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);

  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon, 28, 28);

  /* This gets added to ourself table after our body because of ordering */
  priv->details_overlay = nbtk_table_new ();
  nbtk_widget_set_style_class_name (priv->details_overlay,
                                    "PengePeopleTileDetails");
  clutter_actor_set_opacity ((ClutterActor *)priv->details_overlay, 0x0);

  priv->timeline = clutter_timeline_new_for_duration (300);

  alpha = clutter_alpha_new_full (priv->timeline,
                                  CLUTTER_LINEAR);
  priv->behave = clutter_behaviour_opacity_new (alpha, 0x00, 0xc0);
  clutter_behaviour_apply (priv->behave,
                           (ClutterActor *)priv->details_overlay);

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
                               "row-span",
                               2,
                               "y-expand",
                               FALSE,
                               "x-expand",
                               FALSE,
                               "y-fill",
                               FALSE,
                               "x-fill",
                               FALSE,
                               NULL);

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    self);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    self);

  nbtk_table_set_col_spacing (NBTK_TABLE (priv->details_overlay), 4);
}


