/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright 2012 Intel Corporation.
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
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Lionel Landwerlin  <lionel.g.landwerlin@linux.intel.com>
 *
 */

#define COGL_ENABLE_EXPERIMENTAL_API 1
#define CLUTTER_ENABLE_EXPERIMENTAL_API 1

#include "mnb-home-grid.h"
#include "mnb-home-grid-child.h"
#include "mnb-home-grid-private.h"
#include "mnb-home-widget.h"

#include <math.h>
#include <string.h>

#define SPACING (10)
#define UNIT_SIZE (64)

static void mnb_home_grid_container_iface_init (ClutterContainerIface *iface);
static void mnb_home_grid_scrollable_iface_init (MxScrollableIface *iface);
static void mnb_home_grid_stylable_iface_init (MxStylableIface *iface);
/* static void mnb_home_grid_stylable_iface_init (MxStylableIface *iface); */

G_DEFINE_TYPE_WITH_CODE (MnbHomeGrid, mnb_home_grid, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                mnb_home_grid_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_SCROLLABLE,
                                                mnb_home_grid_scrollable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mnb_home_grid_stylable_iface_init)
                         /* G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE, */
                         /*                        mnb_home_grid_focusable_iface_init) */)

#define GRID_PRIVATE(o)                                                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_HOME_GRID, MnbHomeGridPrivate))

enum
{
  PROP_0,

  PROP_HADJUST,
  PROP_VADJUST,

  PROP_ROWS,
  PROP_COLUMNS,
  PROP_EDIT_MODE,

  PROP_LAST
};

struct _MnbHomeGridPrivate
{
  /* Size of the grid */
  guint cols, rows;

  GList *children;
  GArray *cells; /* TODO?: replace gboolean by actual ClutterActors */

  /* Grid showing available cells */
  CoglMaterial *pipeline;
  float *edition_verts;
  ClutterActor *edit_texture;

  /* Temporary stuff (used to speed up processing on motion events */
  MxPadding tmp_padding;
  gfloat pointer_x, pointer_y;

  /* Selected tile in edition mode */
  ClutterActor *selection;

  /* Visual hint */
  ClutterActor *hint_position;
  gint selection_col, selection_row;
  gint selection_cols, selection_rows;

  gboolean in_edit_mode;

  /**/
  MxAdjustment *hadjustment;
  MxAdjustment *vadjustment;

  guint spacing;
};

enum
{
  DRAG_BEGIN,
  DRAG_END,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static CoglHandle texture_template_pipeline = NULL;

/**/

static void
mnb_home_grid_insert_item_cells (MnbHomeGrid   *grid,
                                 ClutterActor  *child)
{
  MnbHomeGridPrivate *priv = grid->priv;
  MnbHomeGridChild *meta =
    (MnbHomeGridChild *) clutter_container_get_child_meta (CLUTTER_CONTAINER (grid),
                                                           child);
  gint i, j;
  gfloat width, height;

  if (!meta)
    return;

  clutter_actor_get_preferred_size (child, NULL, NULL, &width, &height);

  meta->width = ceilf (width / (UNIT_SIZE + priv->spacing));
  meta->height = ceilf (height / (UNIT_SIZE + priv->spacing));

  for (i = meta->row; i < (meta->row + meta->height); i++)
    {
      for (j = meta->col; j < (meta->col + meta->width); j++)
        {
          ClutterActor **val = &g_array_index (priv->cells,
                                               ClutterActor *,
                                               i * priv->cols + j);
          *val = child;
        }
    }
}

static void
mnb_home_grid_remove_item_cells (MnbHomeGrid   *grid,
                                 ClutterActor *child)
{
  MnbHomeGridPrivate *priv = grid->priv;
  MnbHomeGridChild *meta =
    (MnbHomeGridChild *) clutter_container_get_child_meta (CLUTTER_CONTAINER (grid),
                                                           child);
  gint i, j;

  if (!meta)
    return;

  for (i = meta->row; i < meta->row + meta->height; i++)
    {
      for (j = meta->col; j < meta->col + meta->width; j++)
        {
          ClutterActor **val = &g_array_index (priv->cells,
                                               ClutterActor *,
                                               i * priv->cols + j);
          *val = NULL;
        }
    }
}

static void
mnb_home_grid_reinsert_items_cells (MnbHomeGrid *grid)
{
  MnbHomeGridPrivate *priv = grid->priv;
  gint i;
  GList *child = priv->children;

  for (i = 0; i < priv->cells->len; i++)
    {
      ClutterActor **val = &g_array_index (priv->cells,
                                           ClutterActor *, i);
      *val = NULL;
    }
  while (child)
    {
      mnb_home_grid_insert_item_cells (grid, (ClutterActor *) child->data);
      child = child->next;
    }
}

static gboolean
mnb_home_grid_lookup_position (MnbHomeGrid *grid,
                               ClutterActor *selection,
                               gint col, gint row,
                               gint width, gint height)
{
  MnbHomeGridPrivate *priv = grid->priv;
  gint i, j;

  for (i = row; i < row + height; i++)
    {
      for (j = col; j < col + width; j++)
        {
          ClutterActor *val;
          if ((i >= priv->rows) || (j >= priv->cols))
            return TRUE;

          val = g_array_index (priv->cells,
                               ClutterActor *,
                               i * priv->cols + j);
          if (val && val != selection)
            return TRUE;
        }
    }
  return FALSE;
}

static void
mnb_home_grid_recompute_edition_vertexes (MnbHomeGrid *grid)
{
  MnbHomeGridPrivate *priv = grid->priv;
  gint i, j, k;

  if (priv->edition_verts)
    g_free (priv->edition_verts);
  priv->edition_verts = g_malloc (sizeof (float) * priv->cols * priv->rows * 4);

  for (i = 0, k = 0; i < priv->rows; i++)
    {
      for (j = 0; j < priv->cols; j++)
        {
          priv->edition_verts[k++] = j * (priv->spacing + UNIT_SIZE);
          priv->edition_verts[k++] = i * (priv->spacing + UNIT_SIZE);
          priv->edition_verts[k++] = j * (priv->spacing + UNIT_SIZE) + UNIT_SIZE;
          priv->edition_verts[k++] = i * (priv->spacing + UNIT_SIZE) + UNIT_SIZE;
        }
    }
}

/*
 * Stylable inplementation
 */

static void
mnb_home_grid_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (G_UNLIKELY (!is_initialized))
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_uint ("x-mx-spacing",
                                 "Spacing",
                                 "The size of the spacing",
                                 0, G_MAXUINT, 0,
                                 G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MNB_TYPE_HOME_GRID, pspec);
    }
}

/*
 * MxScrollable Interface Implementation
 */

static void
adjustment_value_notify_cb (MxAdjustment *adjustment,
                            GParamSpec   *pspec,
                            MnbHomeGrid   *grid)
{
  clutter_actor_queue_redraw (CLUTTER_ACTOR (grid));
}

static void
mnb_home_grid_scrollable_set_adjustments (MxScrollable *scrollable,
                                          MxAdjustment *hadjustment,
                                          MxAdjustment *vadjustment)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (scrollable)->priv;

  if (hadjustment != priv->hadjustment)
    {
      if (priv->hadjustment)
        {
          g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                                adjustment_value_notify_cb,
                                                scrollable);
          g_object_unref (priv->hadjustment);
        }

      if (hadjustment)
        {
          g_object_ref (hadjustment);
          g_signal_connect (hadjustment, "notify::value",
                            G_CALLBACK (adjustment_value_notify_cb),
                            scrollable);
        }

      priv->hadjustment = hadjustment;
      g_object_notify (G_OBJECT (scrollable), "horizontal-adjustment");
    }

  if (vadjustment != priv->vadjustment)
    {
      if (priv->vadjustment)
        {
          g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                                adjustment_value_notify_cb,
                                                scrollable);
          g_object_unref (priv->vadjustment);
        }

      if (vadjustment)
        {
          g_object_ref (vadjustment);
          g_signal_connect (vadjustment, "notify::value",
                            G_CALLBACK (adjustment_value_notify_cb),
                            scrollable);
        }

      priv->vadjustment = vadjustment;
      g_object_notify (G_OBJECT (scrollable), "vertical-adjustment");
    }
}

static void
mnb_home_grid_scrollable_get_adjustments (MxScrollable  *scrollable,
                                          MxAdjustment **hadjustment,
                                          MxAdjustment **vadjustment)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (scrollable)->priv;

  if (hadjustment)
    {
      if (priv->hadjustment)
        *hadjustment = priv->hadjustment;
      else
        {
          MxAdjustment *adjustment;

          /* create an initial adjustment. this is filled with correct values
           * as soon as allocate() is called */

          adjustment = mx_adjustment_new ();

          mnb_home_grid_scrollable_set_adjustments (scrollable,
                                                    adjustment,
                                                    priv->vadjustment);

          g_object_unref (adjustment);

          *hadjustment = adjustment;
        }
    }

  if (vadjustment)
    {
      if (priv->vadjustment)
        *vadjustment = priv->vadjustment;
      else
        {
          MxAdjustment *adjustment;

          /* create an initial adjustment. this is filled with correct values
           * as soon as allocate() is called */

          adjustment = mx_adjustment_new ();

          mnb_home_grid_scrollable_set_adjustments (scrollable,
                                                    priv->hadjustment,
                                                    adjustment);

          g_object_unref (adjustment);

          *vadjustment = adjustment;
        }
    }
}

static void
mnb_home_grid_scrollable_iface_init (MxScrollableIface *iface)
{
  iface->set_adjustments = mnb_home_grid_scrollable_set_adjustments;
  iface->get_adjustments = mnb_home_grid_scrollable_get_adjustments;
}

/*
 * ClutterContainer Implementation
 */

static void
mnb_home_grid_add_actor (ClutterContainer *container,
                         ClutterActor     *actor)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (container)->priv;

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));

  priv->children = g_list_append (priv->children, actor);

  g_signal_emit_by_name (container, "actor-added", actor);
}

static void
mnb_home_grid_remove_actor (ClutterContainer *container,
                            ClutterActor     *actor)
{
  MnbHomeGrid *grid = MNB_HOME_GRID (container);
  MnbHomeGridPrivate *priv = grid->priv;
  GList *item = NULL;

  item = g_list_find (priv->children, actor);

  if (item == NULL)
    {
      g_warning ("Widget of type '%s' is not a child of container of type '%s'",
                 g_type_name (G_OBJECT_TYPE (actor)),
                 g_type_name (G_OBJECT_TYPE (container)));
      return;
    }

  g_object_ref (actor);

  mnb_home_grid_remove_item_cells (grid, actor);

  /* if ((ClutterActor *)priv->last_focus == actor) */
  /*   priv->last_focus = NULL; */

  priv->children = g_list_delete_link (priv->children, item);
  clutter_actor_unparent (actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));

  g_signal_emit_by_name (container, "actor-removed", actor);

  g_object_unref (actor);
}

static void
mnb_home_grid_foreach (ClutterContainer *container,
                       ClutterCallback   callback,
                       gpointer          callback_data)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (container)->priv;

  g_list_foreach (priv->children, (GFunc) callback, callback_data);
}

static void
mnb_home_grid_lower (ClutterContainer *container,
                     ClutterActor     *actor,
                     ClutterActor     *sibling)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (container)->priv;
  gint i;
  GList *c, *position, *actor_link = NULL;

  if (priv->children && (priv->children->data == actor))
    return;

  position = priv->children;
  for (c = priv->children, i = 0; c; c = c->next, i++)
    {
      if (c->data == actor)
        actor_link = c;
      if (c->data == sibling)
        position = c;
    }

  if (!actor_link)
    {
      g_warning (G_STRLOC ": Actor of type '%s' is not a child of container "
                 "of type '%s'",
                 g_type_name (G_OBJECT_TYPE (actor)),
                 g_type_name (G_OBJECT_TYPE (container)));
      return;
    }

  priv->children = g_list_delete_link (priv->children, actor_link);
  priv->children = g_list_insert_before (priv->children, position, actor);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (container));
}

static void
mnb_home_grid_raise (ClutterContainer *container,
                     ClutterActor     *actor,
                     ClutterActor     *sibling)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (container)->priv;
  gint i;
  GList *c, *actor_link = NULL;
  gint position = -1;

  for (c = priv->children, i = 0; c; c = c->next, i++)
    {
      if (c->data == actor)
        actor_link = c;
      if (c->data == sibling)
        position = i;
    }

  if (!actor_link)
    {
      g_warning (G_STRLOC ": Actor of type '%s' is not a child of container "
                 "of type '%s'",
                 g_type_name (G_OBJECT_TYPE (actor)),
                 g_type_name (G_OBJECT_TYPE (container)));
      return;
    }

  if (!actor_link->next)
    return;

  priv->children = g_list_delete_link (priv->children, actor_link);
  priv->children = g_list_insert (priv->children, actor, position);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (container));
}

static gint
mnb_home_grid_depth_sort_cb (gconstpointer a,
                             gconstpointer b)
{
  gfloat depth_a = clutter_actor_get_depth ((ClutterActor *) a);
  gfloat depth_b = clutter_actor_get_depth ((ClutterActor *) b);

  if (depth_a < depth_b)
    return -1;
  else if (depth_a > depth_b)
    return 1;
  else
    return 0;
}

static void
mnb_home_grid_sort_depth_order (ClutterContainer *container)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (container)->priv;

  priv->children = g_list_sort (priv->children, mnb_home_grid_depth_sort_cb);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (container));
}

static void
mnb_home_grid_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mnb_home_grid_add_actor;
  iface->remove = mnb_home_grid_remove_actor;
  iface->foreach = mnb_home_grid_foreach;
  iface->lower = mnb_home_grid_lower;
  iface->raise = mnb_home_grid_raise;
  iface->sort_depth_order = mnb_home_grid_sort_depth_order;
  iface->child_meta_type = MNB_TYPE_HOME_GRID_CHILD;
}

/*
 *
 */

static void
mnb_home_grid_show_hint_position (MnbHomeGrid *grid,
                                  gint col, gint row,
                                  gint width, gint height)
{
  MnbHomeGridPrivate *priv = grid->priv;
  gfloat pos_x, pos_y;

  priv->selection_col = col;
  priv->selection_row = row;
  pos_x = priv->tmp_padding.left + (UNIT_SIZE + priv->spacing) * col;
  pos_y = priv->tmp_padding.top + (UNIT_SIZE + priv->spacing) * row;

  clutter_actor_set_position (priv->hint_position, pos_x, pos_y);
  clutter_actor_set_size (priv->hint_position,
                          UNIT_SIZE * width + priv->spacing * (width - 1),
                          UNIT_SIZE * height + priv->spacing * (height - 1));
  clutter_actor_animate (priv->hint_position, CLUTTER_LINEAR, 20,
                         "opacity", 0x80,
                         NULL);
}

static void
mnb_home_grid_hide_hint_position (MnbHomeGrid *grid)
{
  MnbHomeGridPrivate *priv = grid->priv;

  clutter_actor_animate (priv->hint_position, CLUTTER_LINEAR, 20,
                         "opacity", 0x0,
                         NULL);
}

static void
mnb_home_grid_animate_hint_position_if_possible (MnbHomeGrid *grid,
                                                 gint col, gint row)
{
  MnbHomeGridPrivate *priv = grid->priv;
  gfloat pos_x, pos_y;

  if ((priv->selection_col == col) &&
      (priv->selection_row == row))
    return;

  if (mnb_home_grid_lookup_position (grid, priv->selection, col, row,
                                     priv->selection_cols, priv->selection_rows))
    return;

  priv->selection_col = col;
  priv->selection_row = row;
  pos_x = priv->tmp_padding.left + (UNIT_SIZE + priv->spacing) * col;
  pos_y = priv->tmp_padding.top + (UNIT_SIZE + priv->spacing) * row;

  clutter_actor_animate (priv->hint_position, CLUTTER_LINEAR, 20,
                         "x", pos_x,
                         "y", pos_y,
                         NULL);
}

static void
mnb_home_grid_style_changed (MxStylable          *stylable,
                             MxStyleChangedFlags  flags,
                             gpointer             data)
{
  MnbHomeGrid *self = MNB_HOME_GRID (stylable);
  MnbHomeGridPrivate *priv = self->priv;
  guint spacing;

  mx_stylable_get (stylable,
                   "x-mx-spacing", &spacing,
                   NULL);

  if (spacing != priv->spacing)
    {
      priv->spacing = spacing;
      mnb_home_grid_recompute_edition_vertexes (self);
      mnb_home_grid_reinsert_items_cells (self);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
    }
}

static gboolean
stage_motion_event_cb (ClutterActor         *stage,
                       ClutterMotionEvent   *event,
                       MnbHomeGrid           *self)
{
  MnbHomeGridPrivate *priv = self->priv;
  gfloat child_x, child_y;
  gfloat pos_x, pos_y;
  gint col, row;

  child_x = clutter_actor_get_x (priv->selection) + event->x - priv->pointer_x;
  child_y = clutter_actor_get_y (priv->selection) + event->y - priv->pointer_y;

  clutter_actor_set_position (priv->selection, child_x, child_y);

  priv->pointer_x = event->x;
  priv->pointer_y = event->y;

  clutter_actor_get_transformed_position (CLUTTER_ACTOR (self), &pos_x, &pos_y);

  child_x = event->x - pos_x;
  child_y = event->y - pos_y;

  child_x = MIN (child_x, priv->cols * (UNIT_SIZE + priv->spacing) - priv->spacing);
  child_y = MIN (child_y, priv->rows * (UNIT_SIZE + priv->spacing) - priv->spacing);

  col = MAX (0, MIN (child_x / (UNIT_SIZE + priv->spacing),
                     priv->cols - 1));
  row = MAX (0, MIN (child_y / (UNIT_SIZE + priv->spacing),
                     priv->rows - 1));

  col = MIN (col, priv->cols - priv->selection_cols);
  row = MIN (row, priv->rows - priv->selection_rows);

  mnb_home_grid_animate_hint_position_if_possible (self, col, row);

  return TRUE;
}

static gboolean
stage_button_release_event_cb (ClutterActor       *stage,
                               ClutterButtonEvent *event,
                               MnbHomeGrid         *self)
{
  MnbHomeGridPrivate *priv = self->priv;
  MnbHomeGridChild *meta;
  gfloat pos_x, pos_y;

  /* Hide selection hint */
  mnb_home_grid_hide_hint_position (self);

  /* Update selected actor's position in the grid */
  meta = (MnbHomeGridChild *)
    clutter_container_get_child_meta (CLUTTER_CONTAINER (self), priv->selection);
  mnb_home_grid_remove_item_cells (self, priv->selection);
  meta->col = priv->selection_col;
  meta->row = priv->selection_row;
  g_object_set (priv->selection,
      "column", meta->col,
      "row", meta->row,
      NULL);
  mnb_home_grid_insert_item_cells (self, priv->selection);

  /* Animate selected actor to the final position */
  pos_x = priv->tmp_padding.left + (UNIT_SIZE + priv->spacing) * meta->col;
  pos_y = priv->tmp_padding.top + (UNIT_SIZE + priv->spacing) * meta->row;

  clutter_actor_animate (priv->selection, CLUTTER_LINEAR, 200,
                         "x", pos_x,
                         "y", pos_y,
                         NULL);

  g_signal_emit (self, signals[DRAG_END], 0, priv->selection);

  priv->selection = NULL;

  g_signal_handlers_disconnect_by_func (stage,
                                        G_CALLBACK (stage_motion_event_cb),
                                        self);
  g_signal_handlers_disconnect_by_func (stage,
                                        G_CALLBACK (stage_button_release_event_cb),
                                        self);

  return TRUE;
}

/*
 * Actor Implementation
 */

static gboolean
mnb_home_grid_button_press_event (ClutterActor       *self,
                                  ClutterButtonEvent *event)
{
  MnbHomeGrid *grid = MNB_HOME_GRID (self);
  MnbHomeGridPrivate *priv = grid->priv;
  ClutterActor *stage, *child;

  if (!(event->button == 1 && priv->in_edit_mode))
    return
      CLUTTER_ACTOR_CLASS (mnb_home_grid_parent_class)->button_press_event (self,
                                                                            event);

  mx_widget_get_padding (MX_WIDGET (grid), &priv->tmp_padding);

  stage = clutter_actor_get_stage (self);
  child = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage), CLUTTER_PICK_ALL,
                                          event->x, event->y);

  /* Figure out what has been clicked */
  while (child != NULL)
    {
      ClutterActor *parent;

      if (child == self)
        {
          child = NULL;
          break;
        }

      parent = clutter_actor_get_parent (child);

      if ((parent == self) &&
          (child != priv->hint_position))
        break;

      child = parent;
    }

  if (child)
    {
      MnbHomeGridChild *meta = (MnbHomeGridChild *)
        clutter_container_get_child_meta (CLUTTER_CONTAINER (self), child);
      gfloat child_width, child_height;

      /* be sure it's a MnbHomeWidget, it's a programming error if we obtain
       * something else */
      g_assert (MNB_IS_HOME_WIDGET (child));

      priv->pointer_x = event->x;
      priv->pointer_y = event->y;

      priv->selection = child;
      clutter_actor_raise_top (priv->selection);

      clutter_actor_get_size (child, &child_width, &child_height);

      priv->selection_cols = ceilf (child_width / (UNIT_SIZE + priv->spacing));
      priv->selection_rows = ceilf (child_height / (UNIT_SIZE + priv->spacing));

      mnb_home_grid_show_hint_position (grid,
                                        meta->col, meta->row,
                                        priv->selection_cols, priv->selection_rows);

      g_signal_connect (stage, "motion-event",
                        G_CALLBACK (stage_motion_event_cb),
                        self);
      g_signal_connect (stage, "button-release-event",
                        G_CALLBACK (stage_button_release_event_cb),
                        self);

      g_signal_emit (self, signals[DRAG_BEGIN], 0, priv->selection);
    }

  return TRUE;
}

static void
mnb_home_grid_get_preferred_width (ClutterActor *self,
                                   gfloat        for_height,
                                   gfloat       *min_width,
                                   gfloat       *nat_width)
{
  MnbHomeGridPrivate *priv = ((MnbHomeGrid *) self)->priv;
  MxPadding padding;
  gfloat width;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  width = padding.left + padding.right +
    priv->cols * (priv->spacing + UNIT_SIZE) - priv->spacing;

  if (min_width)
    *min_width = width;
  if (nat_width)
    *nat_width = width;
}

static void
mnb_home_grid_get_preferred_height (ClutterActor *self,
                                    gfloat        for_width,
                                    gfloat       *min_height,
                                    gfloat       *nat_height)
{
  MnbHomeGridPrivate *priv = ((MnbHomeGrid *) self)->priv;
  MxPadding padding;
  gfloat height;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  height = padding.top + padding.bottom +
    priv->rows * (priv->spacing + UNIT_SIZE) - priv->spacing;

  if (min_height)
    *min_height = height;
  if (nat_height)
    *nat_height = height;
}

static void
mnb_home_grid_allocate (ClutterActor           *self,
                        const ClutterActorBox  *box,
                        ClutterAllocationFlags  flags)
{
  MnbHomeGrid *grid = MNB_HOME_GRID (self);
  MnbHomeGridPrivate *priv = grid->priv;
  ClutterActorBox child_box;
  MxPadding padding;
  GList *child = priv->children;

  CLUTTER_ACTOR_CLASS (mnb_home_grid_parent_class)->allocate (self, box, flags);

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  while (child)
    {
      ClutterActor *child_actor = (ClutterActor *) child->data;
      ClutterAnimation *animation = clutter_actor_get_animation (child_actor);

      if ((animation == NULL) && (child_actor != priv->selection))
        {
          MnbHomeGridChild *child_meta = (MnbHomeGridChild *)
            clutter_container_get_child_meta (CLUTTER_CONTAINER (self), child_actor);
          gdouble x_align_d, y_align_d;
          MxAlign x_align, y_align;

          x_align_d = child_meta->x_align;
          y_align_d = child_meta->y_align;

          /* Convert to MxAlign */
          if (x_align_d < 1.0 / 3.0)
            x_align = MX_ALIGN_START;
          else if (x_align_d > 2.0 / 3.0)
            x_align = MX_ALIGN_END;
          else
            x_align = MX_ALIGN_MIDDLE;
          if (y_align_d < 1.0 / 3.0)
            y_align = MX_ALIGN_START;
          else if (y_align_d > 2.0 / 3.0)
            y_align = MX_ALIGN_END;
          else
            y_align = MX_ALIGN_MIDDLE;

          child_box.x1 = padding.left + (UNIT_SIZE + priv->spacing) * child_meta->col;
          child_box.y1 = padding.top + (UNIT_SIZE + priv->spacing) * child_meta->row;
          child_box.x2 = child_box.x1 + clutter_actor_get_width (child_actor);
          child_box.y2 = child_box.y1 + clutter_actor_get_height (child_actor);

          mx_allocate_align_fill (child_actor, &child_box,
                                  x_align, y_align,
                                  child_meta->x_fill, child_meta->y_fill);

          clutter_actor_allocate (child_actor, &child_box, flags);
        }
      else
        clutter_actor_allocate_preferred_size (child_actor, flags);

      child = child->next;
    }

  clutter_actor_allocate_preferred_size (priv->hint_position, flags);

  /* update adjustments for scrolling */
  if (priv->vadjustment)
    {
      g_object_set (G_OBJECT (priv->vadjustment),
                    "lower", 0.0,
                    "upper", (padding.top + padding.bottom +
                              (UNIT_SIZE + priv->spacing) * priv->rows - priv->spacing),
                    "page-size", box->y2 - box->y1,
                    "step-increment", 0.0,
                    "page-increment", 0.0,
                    NULL);
    }

  if (priv->hadjustment)
    {
      g_object_set (G_OBJECT (priv->hadjustment),
                    "lower", 0.0,
                    "upper", (padding.left + padding.right +
                              (UNIT_SIZE + priv->spacing) * priv->cols - priv->spacing),
                    "page-size", box->x2 - box->x1,
                    "step-increment", 0.0,
                    "page-increment", 0.0,
                    NULL);
    }
}

static void
mnb_home_grid_apply_transform (ClutterActor *a,
                               CoglMatrix   *m)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (a)->priv;
  gdouble x, y;

  CLUTTER_ACTOR_CLASS (mnb_home_grid_parent_class)->apply_transform (a, m);

  if (priv->hadjustment)
    x = mx_adjustment_get_value (priv->hadjustment);
  else
    x = 0;

  if (priv->vadjustment)
    y = mx_adjustment_get_value (priv->vadjustment);
  else
    y = 0;

  cogl_matrix_translate (m, (int) -x, (int) -y, 0);
}

static gboolean
mnb_home_grid_get_paint_volume (ClutterActor       *actor,
                                ClutterPaintVolume *volume)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (actor)->priv;
  ClutterVertex vertex;

  if (!clutter_paint_volume_set_from_allocation (volume, actor))
    return FALSE;

  clutter_paint_volume_get_origin (volume, &vertex);

  if (priv->hadjustment)
    vertex.x += mx_adjustment_get_value (priv->hadjustment);

  if (priv->vadjustment)
    vertex.y += mx_adjustment_get_value (priv->vadjustment);

  clutter_paint_volume_set_origin (volume, &vertex);

  return TRUE;
}

static void
mnb_home_grid_paint (ClutterActor *self)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (self)->priv;
  GList *l;
  gdouble x, y;
  ClutterActorBox box_b, child_b;

  CLUTTER_ACTOR_CLASS (mnb_home_grid_parent_class)->paint (self);

  if (priv->hadjustment)
    x = mx_adjustment_get_value (priv->hadjustment);
  else
    x = 0;

  if (priv->vadjustment)
    y = mx_adjustment_get_value (priv->vadjustment);
  else
    y = 0;

  clutter_actor_get_allocation_box (self, &box_b);
  box_b.x2 = (box_b.x2 - box_b.x1) + x;
  box_b.x1 = x;
  box_b.y2 = (box_b.y2 - box_b.y1) + y;
  box_b.y1 = y;

  if (priv->in_edit_mode)
    {
      CoglHandle texture =
        mx_widget_get_background_texture (MX_WIDGET (priv->edit_texture));
      guint8 alpha = clutter_actor_get_paint_opacity (priv->edit_texture);

      cogl_material_set_layer (priv->pipeline, 0, texture);
      cogl_material_set_color4ub (priv->pipeline, alpha, alpha, alpha, alpha);
      cogl_set_source (priv->pipeline);
      cogl_rectangles (priv->edition_verts, priv->cols * priv->rows);

      clutter_actor_paint (priv->hint_position);
      for (l = priv->children; l != NULL; l = g_list_next (l))
        {
          ClutterActor *child = CLUTTER_ACTOR (l->data);

          clutter_actor_get_allocation_box (child, &child_b);

          if ((child_b.x1 < box_b.x2) &&
              (child_b.x2 > box_b.x1) &&
              (child_b.y1 < box_b.y2) &&
              (child_b.y2 > box_b.y1))
            {
              clutter_actor_paint (CLUTTER_ACTOR (child));
            }
        }
    }
  else /* !edit_mode */
    {
      for (l = priv->children; l != NULL; l = g_list_next (l))
        {
          ClutterActor *child = CLUTTER_ACTOR (l->data);

          clutter_actor_get_allocation_box (child, &child_b);

          if ((child_b.x1 < box_b.x2) &&
              (child_b.x2 > box_b.x1) &&
              (child_b.y1 < box_b.y2) &&
              (child_b.y2 > box_b.y1))
            {
              clutter_actor_paint (child);
            }
        }
    }
}

static void
mnb_home_grid_pick (ClutterActor       *self,
                    const ClutterColor *color)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (self)->priv;
  GList *l;
  gdouble x, y;
  ClutterActorBox box_b, child_b;

  CLUTTER_ACTOR_CLASS (mnb_home_grid_parent_class)->pick (self, color);

  if (priv->hadjustment)
    x = mx_adjustment_get_value (priv->hadjustment);
  else
    x = 0;

  if (priv->vadjustment)
    y = mx_adjustment_get_value (priv->vadjustment);
  else
    y = 0;

  clutter_actor_get_allocation_box (self, &box_b);
  box_b.x2 = (box_b.x2 - box_b.x1) + x;
  box_b.x1 = x;
  box_b.y2 = (box_b.y2 - box_b.y1) + y;
  box_b.y1 = y;

  if (priv->in_edit_mode)
    {
      cogl_rectangles (priv->edition_verts, priv->cols * priv->rows);

      clutter_actor_paint (priv->hint_position);
      for (l = priv->children; l != NULL; l = g_list_next (l))
        {
          ClutterActor *child = CLUTTER_ACTOR (l->data);

          clutter_actor_get_allocation_box (child, &child_b);

          if ((child_b.x1 < box_b.x2) &&
              (child_b.x2 > box_b.x1) &&
              (child_b.y1 < box_b.y2) &&
              (child_b.y2 > box_b.y1))
            {
              clutter_actor_paint (child);
            }
        }
    }
  else
    {
      for (l = priv->children; l != NULL; l = g_list_next (l))
        {
          ClutterActor *child = CLUTTER_ACTOR (l->data);

          clutter_actor_get_allocation_box (child, &child_b);

          if ((child_b.x1 < box_b.x2) &&
              (child_b.x2 > box_b.x1) &&
              (child_b.y1 < box_b.y2) &&
              (child_b.y2 > box_b.y1))
            {
              clutter_actor_paint (child);
            }
        }
    }
}

/*
 * Object Implementation
 */

static void
mnb_home_grid_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (object)->priv;
  MxAdjustment *adjustment;

  switch (property_id)
    {
    case PROP_HADJUST:
      mnb_home_grid_scrollable_get_adjustments (MX_SCROLLABLE (object),
                                                &adjustment, NULL);
      g_value_set_object (value, adjustment);
      break;

    case PROP_VADJUST:
      mnb_home_grid_scrollable_get_adjustments (MX_SCROLLABLE (object),
                                                NULL, &adjustment);
      g_value_set_object (value, adjustment);
      break;

    case PROP_ROWS:
      g_value_set_uint (value, priv->rows);
      break;

    case PROP_COLUMNS:
      g_value_set_uint (value, priv->cols);
      break;

    case PROP_EDIT_MODE:
      g_value_set_boolean (value, priv->in_edit_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_home_grid_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MnbHomeGrid *self = MNB_HOME_GRID (object);
  MnbHomeGridPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_HADJUST:
      mnb_home_grid_scrollable_set_adjustments (MX_SCROLLABLE (object),
                                                g_value_get_object (value),
                                                priv->vadjustment);
      break;

    case PROP_VADJUST:
      mnb_home_grid_scrollable_set_adjustments (MX_SCROLLABLE (object),
                                                priv->hadjustment,
                                                g_value_get_object (value));
      break;

    case PROP_ROWS:
      mnb_home_grid_set_grid_size (self, priv->cols, g_value_get_uint (value));
      break;

    case PROP_COLUMNS:
      mnb_home_grid_set_grid_size (self, g_value_get_uint (value), priv->rows);
      break;

    case PROP_EDIT_MODE:
      mnb_home_grid_set_edit_mode (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_home_grid_dispose (GObject *object)
{
  MnbHomeGridPrivate *priv = MNB_HOME_GRID (object)->priv;

  if (priv->pipeline != NULL)
    {
      cogl_object_unref (priv->pipeline);
      priv->pipeline = NULL;
    }

  G_OBJECT_CLASS (mnb_home_grid_parent_class)->dispose (object);
}

static void
mnb_home_grid_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_home_grid_parent_class)->finalize (object);
}

/**/

static void
mnb_home_grid_class_init (MnbHomeGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbHomeGridPrivate));

  object_class->get_property = mnb_home_grid_get_property;
  object_class->set_property = mnb_home_grid_set_property;
  object_class->dispose = mnb_home_grid_dispose;
  object_class->finalize = mnb_home_grid_finalize;

  actor_class->button_press_event = mnb_home_grid_button_press_event;
  actor_class->get_preferred_width = mnb_home_grid_get_preferred_width;
  actor_class->get_preferred_height = mnb_home_grid_get_preferred_height;
  actor_class->allocate = mnb_home_grid_allocate;
  actor_class->apply_transform = mnb_home_grid_apply_transform;
  actor_class->paint = mnb_home_grid_paint;
  actor_class->get_paint_volume = mnb_home_grid_get_paint_volume;
  actor_class->pick = mnb_home_grid_pick;

  /* MxScrollable properties */
  g_object_class_override_property (object_class,
                                    PROP_HADJUST,
                                    "horizontal-adjustment");

  g_object_class_override_property (object_class,
                                    PROP_VADJUST,
                                    "vertical-adjustment");

  g_object_class_install_property (object_class, PROP_ROWS,
                                   g_param_spec_uint ("rows",
                                                      "Row",
                                                      "",
                                                      0, G_MAXUINT, 3,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_COLUMNS,
                                   g_param_spec_uint ("columns",
                                                      "Column",
                                                      "",
                                                      0, G_MAXUINT, 5,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_EDIT_MODE,
                                   g_param_spec_boolean ("edit-mode",
                                                         "Edit Mode",
                                                         "%TRUE if we are editing the settings for this widget",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  signals[DRAG_BEGIN] =
    g_signal_new ("drag-begin",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, CLUTTER_TYPE_ACTOR);

  signals[DRAG_END] =
    g_signal_new ("drag-end",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, CLUTTER_TYPE_ACTOR);
}

static void
mnb_home_grid_init (MnbHomeGrid *self)
{
  MnbHomeGridPrivate *priv;
  ClutterActor *actor;

  if (G_UNLIKELY (texture_template_pipeline == NULL))
    {
      CoglPipeline *pipeline;
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());

      texture_template_pipeline = cogl_pipeline_new (ctx);
      pipeline = COGL_PIPELINE (texture_template_pipeline);
      cogl_pipeline_set_layer_null_texture (pipeline,
                                            0, /* layer_index */
                                            COGL_TEXTURE_TYPE_2D);
    }

  priv = self->priv = GRID_PRIVATE (self);
  actor = CLUTTER_ACTOR (self);

  priv->pipeline = (CoglHandle) cogl_pipeline_copy (texture_template_pipeline);

  clutter_actor_set_reactive (actor, TRUE);

  priv->hint_position = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->hint_position),
                               "HomeGridHint");
  clutter_actor_add_child (actor, priv->hint_position);
  clutter_actor_set_opacity (priv->hint_position, 0x0);

  priv->edit_texture = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->edit_texture),
                               "HomeGridBackgroundTile");
  clutter_actor_add_child (actor, priv->edit_texture);

  priv->cols = priv->rows = 1;
  priv->cells = g_array_new (FALSE, TRUE, sizeof (ClutterActor *));
  g_array_set_size (priv->cells, priv->cols * priv->rows);

  mnb_home_grid_recompute_edition_vertexes (self);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mnb_home_grid_style_changed), NULL);
}

ClutterActor *
mnb_home_grid_new (void)
{
  return g_object_new (MNB_TYPE_HOME_GRID, NULL);
}

/**
 * mnb_home_grid_set_edit_mode:
 * @grid: a #MnbHomeGrid
 * @value:
 *
 * Set the grid in edition mode.
 */
void
mnb_home_grid_set_edit_mode (MnbHomeGrid *self, gboolean value)
{
  MnbHomeGridPrivate *priv;

  g_return_if_fail (MNB_IS_HOME_GRID (self));

  priv = self->priv;

  if (priv->in_edit_mode == value)
    return;

  priv->in_edit_mode = value;
  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

/**
 * mnb_home_grid_get_edit_mode:
 * @grid: a #MnbHomeGrid
 *
 * Retrieves the edition mode.
 *
 * Return value: %TRUE if the grid is in edition mode, %FALSE
 * otherwise.
 */
gboolean
mnb_home_grid_get_edit_mode (MnbHomeGrid *self)
{
  g_return_val_if_fail (MNB_IS_HOME_GRID (self), FALSE);

  return self->priv->in_edit_mode;
}

/**
 * mnb_home_grid_insert_actor:
 * @grid: a #MnbHomeGrid
 * @actor: the child to insert
 * @row: the row to place the child into
 * @col: the column to place the child into
 *
 * Add an actor at the specified row and column
 *
 * Note, column and rows numbers start from zero
 */

/* TODO: might need to return a boolean if insertion is not
   possible. */
void
mnb_home_grid_insert_actor (MnbHomeGrid  *self,
                            ClutterActor *actor,
                            gint          col,
                            gint          row)
{
  MnbHomeGridPrivate *priv;
  ClutterContainer *container;
  MnbHomeGridChild *meta;
  gfloat width, height;

  g_return_if_fail (MNB_IS_HOME_GRID (self));
  g_return_if_fail (MNB_IS_HOME_WIDGET (actor));

  priv = self->priv;

  g_return_if_fail (col >= 0 && col < priv->cols);
  g_return_if_fail (row >= 0 && row < priv->rows);

  container = CLUTTER_CONTAINER (self);
  clutter_container_add_actor (container, actor);
  clutter_actor_get_preferred_size (actor, NULL, NULL, &width, &height);

  meta = (MnbHomeGridChild *) clutter_container_get_child_meta (container,
                                                                actor);
  meta->row = row;
  meta->col = col;
  meta->width = ceilf (width / (UNIT_SIZE + priv->spacing));
  meta->height = ceilf (height / (UNIT_SIZE + priv->spacing));
  mnb_home_grid_insert_item_cells (self, actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
}

/**
 * mnb_home_grid_set_grid_size:
 * @grid: a #MnbHomeGrid
 * @cols: the number of columns
 * @rows: the number of rows
 *
 * Change the size of a #MnbHomeGrid.
 */
void
mnb_home_grid_set_grid_size (MnbHomeGrid *self, guint cols, guint rows)
{
  MnbHomeGridPrivate *priv;

  g_return_if_fail (MNB_IS_HOME_GRID (self));
  g_return_if_fail (cols > 0 && rows > 0);

  priv = self->priv;

  if ((cols == priv->cols) && (rows == priv->rows))
    return;

  priv->cols = cols;
  priv->rows = rows;

  g_array_set_size (priv->cells, priv->cols * priv->rows);
  memset (priv->cells->data, 0,
          priv->cells->len * sizeof (ClutterActor *));

  mnb_home_grid_reinsert_items_cells (self);
  mnb_home_grid_recompute_edition_vertexes (self);
}

/**
 * mnb_home_grid_get_grid_size:
 * @grid: a #MnbHomeGrid
 * @cols: (out): the number of columns
 * @rows: (out): the number of rows
 *
 * Get the size of a #MnbHomeGrid.
 */
void
mnb_home_grid_get_grid_size (MnbHomeGrid *self, guint *cols, guint *rows)
{
  MnbHomeGridPrivate *priv;

  g_return_if_fail (MNB_IS_HOME_GRID (self));

  priv = self->priv;

  if (cols)
    *cols = priv->cols;
  if (rows)
    *rows = priv->rows;
}
