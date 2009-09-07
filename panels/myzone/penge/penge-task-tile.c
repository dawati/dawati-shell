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


#include "penge-task-tile.h"
#include "penge-utils.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (PengeTaskTile, penge_task_tile, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_TASK_TILE, PengeTaskTilePrivate))

typedef struct _PengeTaskTilePrivate PengeTaskTilePrivate;

struct _PengeTaskTilePrivate {
    JanaTask *task;
    JanaStore *store;

    NbtkWidget *summary_label;
    NbtkWidget *details_label;
    NbtkWidget *check_button;

    guint commit_timeout;

    NbtkWidget *inner_table;
};

enum
{
  PROP_0,
  PROP_TASK,
  PROP_STORE
};

static void penge_task_tile_update (PengeTaskTile *tile);
static gboolean _commit_timeout_cb (gpointer userdata);

static void
penge_task_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TASK:
      g_value_set_object (value, priv->task);
      break;
    case PROP_STORE:
      g_value_set_object (value, priv->store);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_task_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TASK:
      if (priv->task)
        g_object_unref (priv->task);

      priv->task = g_value_dup_object (value);

      penge_task_tile_update ((PengeTaskTile *)object);
      break;
    case PROP_STORE:
      priv->store = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_task_tile_dispose (GObject *object)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (object);

  if (priv->commit_timeout)
  {
    g_source_remove (priv->commit_timeout);
    _commit_timeout_cb (object);
  }

  if (priv->task)
  {
    g_object_unref (priv->task);
    priv->task = NULL;
  }

  G_OBJECT_CLASS (penge_task_tile_parent_class)->dispose (object);
}

static void
penge_task_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_task_tile_parent_class)->finalize (object);
}

static void
penge_task_tile_class_init (PengeTaskTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeTaskTilePrivate));

  object_class->get_property = penge_task_tile_get_property;
  object_class->set_property = penge_task_tile_set_property;
  object_class->dispose = penge_task_tile_dispose;
  object_class->finalize = penge_task_tile_finalize;

  pspec = g_param_spec_object ("task",
                               "The task",
                               "The task to show.",
                               JANA_TYPE_TASK,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TASK, pspec);

  pspec = g_param_spec_object ("store",
                               "The store.",
                               "The store this task came from.",
                               JANA_ECAL_TYPE_STORE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_STORE, pspec);
}

static gboolean
_commit_timeout_cb (gpointer userdata)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (userdata);

  jana_store_modify_component (priv->store, JANA_COMPONENT (priv->task));

  priv->commit_timeout = 0;
  return FALSE;
}

static void
_check_button_clicked_cb (NbtkButton *button,
                          gpointer    userdata)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (userdata);

  if (nbtk_button_get_checked (button))
  {
    jana_task_set_completed (priv->task, TRUE);
  } else {
    jana_task_set_completed (priv->task, FALSE);
  }

  if (priv->commit_timeout)
  {
    g_source_remove (priv->commit_timeout);
  }

  priv->commit_timeout = g_timeout_add_seconds (1,
                                                _commit_timeout_cb,
                                                userdata);
}

static void
_button_clicked_cb (NbtkButton *button,
                    gpointer    userdata)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (userdata);
  gchar *uid;
  gchar *command_line;

  uid = jana_component_get_uid ((JanaComponent *)priv->task);

  command_line = g_strdup_printf ("tasks --edit=\"%s\"",
                                  uid);
  g_free (uid);

  if (!penge_utils_launch_by_command_line ((ClutterActor *)button,
                                           command_line))
  {
    g_warning (G_STRLOC ": Error starting tasks");
  } else{
    penge_utils_signal_activated ((ClutterActor *)userdata);
  }
}

static void
penge_task_tile_init (PengeTaskTile *self)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;

  priv->inner_table = nbtk_table_new ();
  nbtk_bin_set_child (NBTK_BIN (self), (ClutterActor *)priv->inner_table);
  nbtk_bin_set_fill (NBTK_BIN (self), TRUE, TRUE);

  priv->check_button = nbtk_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (priv->check_button), TRUE);
  nbtk_widget_set_style_class_name (priv->check_button,
                                    "PengeTaskToggleButton");
  clutter_actor_set_size ((ClutterActor *)priv->check_button, 21, 21);

  priv->summary_label = nbtk_label_new ("Summary text");
  nbtk_widget_set_style_class_name (priv->summary_label,
                                    "PengeTaskSummaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->summary_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);

  priv->details_label = nbtk_label_new ("Details text");
  nbtk_widget_set_style_class_name (priv->details_label,
                                    "PengeTaskDetails");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->details_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);

  /* Populate the table */
  nbtk_table_add_actor (NBTK_TABLE (priv->inner_table),
                        (ClutterActor *)priv->check_button,
                        0,
                        0);
  nbtk_table_add_actor (NBTK_TABLE (priv->inner_table),
                        (ClutterActor *)priv->summary_label,
                        0,
                        1);
  nbtk_table_add_actor (NBTK_TABLE (priv->inner_table),
                        (ClutterActor *)priv->details_label,
                        1,
                        1);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               (ClutterActor *)priv->check_button,
                               "x-expand",
                               FALSE,
                               "x-fill",
                               FALSE,
                               "y-expand",
                               FALSE,
                               "y-fill",
                               FALSE,
                               "row-span",
                               2,
                               NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               (ClutterActor *)priv->summary_label,
                               "x-expand",
                               TRUE,
                               "y-fill",
                               FALSE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               (ClutterActor *)priv->details_label,
                               "x-expand",
                               TRUE,
                               "y-fill",
                               FALSE,
                               NULL);

  /* Setup spacing and padding */
  nbtk_table_set_row_spacing (NBTK_TABLE (priv->inner_table), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (priv->inner_table), 8);

  g_signal_connect (priv->check_button,
                   "clicked",
                   (GCallback)_check_button_clicked_cb,
                   self);

  g_signal_connect (self,
                    "clicked",
                    (GCallback)_button_clicked_cb,
                    self);

  clutter_actor_set_reactive ((ClutterActor *)self, TRUE);
}

static void
penge_task_tile_update (PengeTaskTile *tile)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (tile);
  gchar *summary_str, *details_str;
  JanaTime *due;

  if (!priv->task)
    return;

  summary_str = jana_task_get_summary (priv->task);

  if (summary_str)
  {
    nbtk_label_set_text (NBTK_LABEL (priv->summary_label), summary_str);
    g_free (summary_str);
  } else {
    nbtk_label_set_text (NBTK_LABEL (priv->summary_label), "");
    g_warning (G_STRLOC ": No summary string for task.");
  }

  due = jana_task_get_due_date (priv->task);

  if (due)
  {
    details_str = jana_utils_strftime (due, _("Due %x"));
    nbtk_label_set_text (NBTK_LABEL(priv->details_label), details_str);
    g_free (details_str);

    clutter_actor_show (CLUTTER_ACTOR (priv->details_label));
    clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                                 (ClutterActor *)priv->summary_label,
                                 "row-span",
                                 1,
                                 NULL);
  } else {
    /* 
     * If we fail to get some kind of description make the summary text
     * cover both rows in the tile
     */
    clutter_actor_hide (CLUTTER_ACTOR (priv->details_label));
    clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                                 (ClutterActor *)priv->summary_label,
                                 "row-span",
                                 2,
                                 NULL);
  }

  nbtk_button_set_checked (NBTK_BUTTON (priv->check_button),
                           jana_task_get_completed (priv->task));
}

gchar *
penge_task_tile_get_uid (PengeTaskTile *tile)
{
  PengeTaskTilePrivate *priv = GET_PRIVATE (tile);

  return jana_component_get_uid (JANA_COMPONENT (priv->task));
}

