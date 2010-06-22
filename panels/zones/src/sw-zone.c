/*
 * Copyright © 2009, 2010, Intel Corporation.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors: Thomas Wood <thomas.wood@intel.com>
 */

#include "sw-zone.h"
#include "sw-window.h"
#include <mx/mx.h>

#include <glib/gi18n.h>

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_droppable_iface_init (MxDroppableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (SwZone, sw_zone, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER, clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DROPPABLE, mx_droppable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE, mx_focusable_iface_init))

#define ZONE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SW_TYPE_ZONE, SwZonePrivate))

#define PADDING 6
#define MIN_SIZE 100

#define DUMMY_MAX_WIDTH 200.0
#define DUMMY_MIN_WIDTH 100.0

#define WINDOW_EDGE_PADDING 18.0

enum
{
  PROP_DUMMY = 1,
  PROP_ENABLED
};

struct _SwZonePrivate
{
  guint is_enabled : 1;
  guint dummy : 1;
  guint focused : 1;
  guint drag_in_progress : 1;
  gfloat dummy_size;

  ClutterActor *placeholder;

  GList *children;
  gint n_children;

  GList *start_allocations;
  GList *end_allocations;
  gboolean is_animating;
  ClutterTimeline *timeline;
  ClutterAlpha *alpha;

  ClutterActor *title;
  ClutterActor *add_icon;
  ClutterActor *label;

  ClutterActor *welcome;
  gboolean is_welcome;

  gint number;
};


/* focusable implementation */
static MxFocusable *
sw_zone_accept_focus (MxFocusable *focusable,
                      MxFocusHint  hint)
{
  mx_stylable_set_style_pseudo_class(MX_STYLABLE (focusable), "active");
  return focusable;
}

static MxFocusable *
sw_zone_move_focus (MxFocusable      *focusable,
                    MxFocusDirection  direction,
                    MxFocusable      *from)
{
  GList *children;

  if (direction == MX_FOCUS_DIRECTION_LEFT
      || direction == MX_FOCUS_DIRECTION_RIGHT)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (focusable), "");
      return NULL;
    }

  children = SW_ZONE (focusable)->priv->children;

  if (!children)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (focusable), "");
      return NULL;
    }

  if (from == focusable)
    mx_stylable_set_style_pseudo_class (MX_STYLABLE (focusable), "");

    {
      GList *l;
      for (l = SW_ZONE (focusable)->priv->children; l; l = l->next)
        {
          if (l->data == from)
            {
              GList *next;
              if (direction == MX_FOCUS_DIRECTION_DOWN
                  || direction == MX_FOCUS_DIRECTION_NEXT)
                next = l->next;
              else
                next = l->prev;

              if (next)
                return mx_focusable_accept_focus (next->data, 0);
              else
                return NULL;
            }
        }
    }

  /* focus first child */
  if (direction == MX_FOCUS_DIRECTION_UP)
    return mx_focusable_accept_focus (g_list_last (children)->data, 0);
  else
    return mx_focusable_accept_focus (children->data, 0);
}


static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = sw_zone_accept_focus;
  iface->move_focus = sw_zone_move_focus;
}


void sw_zone_finish_animation (SwZone *zone);


static void
sw_zone_update_title (SwZone *zone)
{
  SwZonePrivate *priv = SW_ZONE (zone)->priv;
  MxLabel *label = MX_LABEL (priv->title);

  if (priv->dummy)
    {
      mx_label_set_text (label, _("New Zone"));
    }
  else if (priv->n_children > 0)
    {
      gchar *zone_title;

      zone_title = g_strdup_printf (_("Zone %d"), priv->number);
      mx_label_set_text (label, zone_title);
      g_free (zone_title);
    }
  else
    mx_label_set_text (label, "");
}

void
sw_zone_start_animation (SwZone *zone)
{
  SwZonePrivate *priv = zone->priv;

  if (priv->is_animating
      || !CLUTTER_ACTOR_IS_MAPPED (CLUTTER_ACTOR (zone)))
      return;

  priv->is_animating = TRUE;

  priv->timeline = clutter_timeline_new (200);
  g_signal_connect_swapped (priv->timeline, "new-frame",
                            G_CALLBACK (clutter_actor_queue_relayout), zone);
  g_signal_connect_swapped (priv->timeline, "completed",
                            G_CALLBACK (sw_zone_finish_animation), zone);

  priv->alpha = clutter_alpha_new_full (priv->timeline,
                                        CLUTTER_EASE_OUT_CUBIC);

  clutter_timeline_start (priv->timeline);
}

void
sw_zone_finish_animation (SwZone *zone)
{
  SwZonePrivate *priv = zone->priv;

  if (priv->start_allocations)
    {
      g_list_foreach (priv->start_allocations, (GFunc) g_free, NULL);
      g_list_free (priv->start_allocations);
      priv->start_allocations = NULL;
    }

  if (priv->end_allocations)
    {
      g_list_foreach (priv->end_allocations, (GFunc) g_free, NULL);
      g_list_free (priv->end_allocations);
      priv->end_allocations = NULL;
    }

  if (priv->timeline)
    {
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  if (priv->alpha)
    {
      g_object_unref (priv->alpha);
      priv->alpha = NULL;
    }

  priv->is_animating = FALSE;

  if (priv->placeholder)
    {
      clutter_actor_animate (priv->placeholder, CLUTTER_LINEAR, 100,
                             "opacity", 128, NULL);
    }
}

static void
sw_zone_add (ClutterContainer *container,
             ClutterActor     *actor)
{
  SwZonePrivate *priv = SW_ZONE (container)->priv;

  priv->children = g_list_append (priv->children, actor);
  priv->n_children++;

  sw_zone_update_title (SW_ZONE (container));

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));

  if (priv->placeholder != actor)
    g_signal_emit_by_name (container, "actor-added", actor);

  sw_zone_start_animation (SW_ZONE (container));
}

static void
sw_zone_remove (ClutterContainer *container,
                ClutterActor     *actor)
{
  SwZonePrivate *priv = SW_ZONE (container)->priv;

  priv->children = g_list_remove (priv->children, actor);
  priv->n_children--;

  sw_zone_update_title (SW_ZONE (container));

  clutter_actor_unparent (actor);

  if (priv->n_children > 0)
    sw_zone_start_animation (SW_ZONE (container));
}


static void
sw_zone_foreach (ClutterContainer *container,
                 ClutterCallback   callback,
                 gpointer          data)
{
  SwZonePrivate *priv = SW_ZONE (container)->priv;

  g_list_foreach (priv->children, (GFunc) callback, data);
}


static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = sw_zone_add;
  iface->remove = sw_zone_remove;
  iface->foreach = sw_zone_foreach;
}

#if 0
static void
sw_zone_enable (MxDroppable *droppable)
{
  /* XXX */
}

static void
sw_zone_disable (MxDroppable *droppable)
{
  /* XXX */
}
#endif

static gboolean
sw_zone_accept_drop (MxDroppable *droppable,
                     MxDraggable *draggable)
{
  SwZonePrivate *priv = SW_ZONE (droppable)->priv;

  if (!g_list_find (priv->children, draggable))
    return TRUE;
  else
    return FALSE;
}

static void
sw_zone_over_in (MxDroppable *droppable,
                 MxDraggable *draggable)
{
  SwZonePrivate *priv = SW_ZONE (droppable)->priv;

  if (priv->dummy)
    {
      ClutterActor *parent = clutter_actor_get_parent (CLUTTER_ACTOR (droppable));
      clutter_container_child_set (CLUTTER_CONTAINER (parent),
                                   CLUTTER_ACTOR (droppable),
                                   "expand", TRUE, NULL);
    }

  /* add the placeholder */
  priv->placeholder = sw_window_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->placeholder),
                               "placeholder");
  clutter_actor_set_opacity (priv->placeholder, 0);
  clutter_actor_set_reactive (priv->placeholder, FALSE);
  clutter_container_add_actor (CLUTTER_CONTAINER (droppable),
                               priv->placeholder);

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (droppable), "active");
}

static void
sw_zone_over_out (MxDroppable *droppable,
                  MxDraggable *draggable)
{
  SwZonePrivate *priv = SW_ZONE (droppable)->priv;

  clutter_actor_destroy (priv->placeholder);
  priv->placeholder = NULL;


  if (priv->dummy)
    {
      ClutterActor *parent = clutter_actor_get_parent (CLUTTER_ACTOR (droppable));
      clutter_container_child_set (CLUTTER_CONTAINER (parent),
                                   CLUTTER_ACTOR (droppable),
                                   "expand", FALSE, NULL);
    }

  mx_stylable_set_style_pseudo_class (MX_STYLABLE (droppable), "");

}

static void
sw_zone_drop (MxDroppable         *droppable,
              MxDraggable         *draggable,
              gfloat               event_x,
              gfloat               event_y,
              gint                 button,
              ClutterModifierType  modifiers)
{
  SwZonePrivate *priv = SW_ZONE (droppable)->priv;
  SwZone *old_zone;

  clutter_actor_destroy (priv->placeholder);
  priv->placeholder = NULL;

  old_zone = SW_ZONE (clutter_actor_get_parent (CLUTTER_ACTOR (draggable)));

  clutter_actor_reparent (CLUTTER_ACTOR (draggable),
                          CLUTTER_ACTOR (droppable));

  if (old_zone->priv->n_children == 0)
    clutter_actor_destroy (CLUTTER_ACTOR (old_zone));

  sw_zone_set_dummy (SW_ZONE (droppable), FALSE);
  clutter_actor_animate (CLUTTER_ACTOR (droppable),
                         CLUTTER_EASE_OUT_QUAD, 200,
                         "opacity", 0xff, NULL);

}

static void
mx_droppable_iface_init (MxDroppableIface *iface)
{

  iface->accept_drop = sw_zone_accept_drop;

  iface->over_in = sw_zone_over_in;
  iface->over_out = sw_zone_over_out;
  iface->drop = sw_zone_drop;
}


static void
sw_zone_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  SwZonePrivate *priv = SW_ZONE (object)->priv;

  switch (property_id)
    {
  case PROP_DUMMY:
    g_value_set_boolean (value, priv->dummy);
    break;

  case PROP_ENABLED:
    g_value_set_boolean (value, priv->is_enabled);
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sw_zone_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  SwZonePrivate *priv = SW_ZONE (object)->priv;

  switch (property_id)
    {
  case PROP_DUMMY:
    sw_zone_set_dummy (SW_ZONE (object), g_value_get_boolean (value));
    break;

  case PROP_ENABLED:
    priv->is_enabled = g_value_get_boolean (value);
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sw_zone_dispose (GObject *object)
{
  SwZonePrivate *priv = SW_ZONE (object)->priv;
  GList *l;

  if (priv->children)
    {
      for (l = priv->children; l; l = g_list_next (l))
        {
          /* call unparent to make sure no animation is run */
          clutter_actor_unparent (CLUTTER_ACTOR (l->data));
        }
      g_list_free (priv->children);
      priv->children = NULL;
    }

  if (priv->label)
    {
      clutter_actor_unparent (priv->label);
      priv->label = NULL;
    }

  if (priv->add_icon)
    {
      clutter_actor_unparent (priv->add_icon);
      priv->add_icon = NULL;
    }

  if (priv->title)
    {
      clutter_actor_unparent (priv->title);
      priv->title = NULL;
    }

  if (priv->timeline)
    {
      g_object_unref (priv->timeline);
      priv->timeline = NULL;
    }

  if (priv->placeholder)
    {
      clutter_actor_unparent (priv->placeholder);
      priv->placeholder = NULL;
    }

  if (priv->alpha)
    {
      g_object_unref (priv->alpha);
      priv->alpha = NULL;
    }


  G_OBJECT_CLASS (sw_zone_parent_class)->dispose (object);
}

static void
sw_zone_finalize (GObject *object)
{
  SwZonePrivate *priv = SW_ZONE (object)->priv;

  if (priv->start_allocations)
    {
      g_list_foreach (priv->start_allocations, (GFunc) g_free, NULL);
      g_list_free (priv->start_allocations);
      priv->start_allocations = NULL;
    }

  if (priv->end_allocations)
    {
      g_list_foreach (priv->end_allocations, (GFunc) g_free, NULL);
      g_list_free (priv->end_allocations);
      priv->end_allocations = NULL;
    }

  G_OBJECT_CLASS (sw_zone_parent_class)->finalize (object);
}

static void
sw_zone_get_preferred_width (ClutterActor *actor,
                             gfloat        for_height,
                             gfloat       *min_width,
                             gfloat       *pref_width)
{
  SwZonePrivate *priv = SW_ZONE (actor)->priv;
  MxPadding padding;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (priv->dummy)
    {
      clutter_actor_get_preferred_width (priv->title, -1,
                                         min_width, pref_width);
      if (min_width)
        *min_width += padding.left + padding.right;

      if (pref_width)
        *pref_width += padding.left + padding.right;
    }
  else
    {
      if (min_width)
       *min_width = MIN_SIZE;

      if (pref_width)
       *pref_width = MIN_SIZE;
    }
}

static void
sw_zone_get_preferred_height (ClutterActor *actor,
                              gfloat        for_width,
                              gfloat       *min_height,
                              gfloat       *pref_height)
{
  SwZonePrivate *priv = SW_ZONE (actor)->priv;

  if (min_height)
    *min_height = 0;

  if (pref_height)
    {
      GList *l;

      *pref_height = 0;

      for (l = priv->children; l; l = l->next)
        {
          gfloat height;

          clutter_actor_get_preferred_height (CLUTTER_ACTOR (l->data),
                                              for_width,
                                              NULL,
                                              &height);

          *pref_height += height + PADDING;
        }
    }
}

static void
sw_zone_allocate (ClutterActor           *actor,
                  const ClutterActorBox  *box,
                  ClutterAllocationFlags  flags)
{
  GList *l;
  ClutterActorBox avail_box;
  gint n = 0;
  gfloat title_height;
  ClutterActorBox childbox;

  SwZonePrivate *priv = SW_ZONE (actor)->priv;

  CLUTTER_ACTOR_CLASS (sw_zone_parent_class)->allocate (actor, box, flags);

  gfloat start;
  gfloat child_height;

  mx_widget_get_available_area (MX_WIDGET (actor), box, &avail_box);

  clutter_actor_get_preferred_height (priv->title,
                                      (avail_box.x2 - avail_box.x1),
                                      NULL,
                                      &title_height);

  childbox.x1 = avail_box.x1;
  childbox.y1 = avail_box.y1;
  childbox.x2 = avail_box.x2;
  childbox.y2 = avail_box.y1 + title_height;
  clutter_actor_allocate (priv->title, &childbox, flags);

  /* remove the title from the available box */
  avail_box.y1 += title_height + PADDING;


  if (priv->dummy)
    {
      childbox = avail_box;

      avail_box.x1 += WINDOW_EDGE_PADDING;
      avail_box.x2 -= WINDOW_EDGE_PADDING;

      mx_allocate_align_fill (priv->add_icon, &childbox, MX_ALIGN_MIDDLE,
                              MX_ALIGN_MIDDLE, FALSE, FALSE);
      clutter_actor_allocate (priv->add_icon, &childbox, flags);

      if (priv->placeholder)
        {
          childbox = avail_box;
          mx_allocate_align_fill (priv->placeholder, &childbox,
                                  MX_ALIGN_MIDDLE, MX_ALIGN_MIDDLE, FALSE,
                                  FALSE);
          clutter_actor_allocate (priv->placeholder, &childbox, flags);
        }

      return;
    }

  if (priv->label)
    {
      childbox = avail_box;
      mx_allocate_align_fill (priv->label, &childbox, MX_ALIGN_MIDDLE,
                              MX_ALIGN_MIDDLE, FALSE, FALSE);
      clutter_actor_allocate (priv->label, &childbox, flags);
    }

  if (priv->welcome)
    {
      childbox = avail_box;
      childbox.x1 += 17;
      childbox.y1 += 37;
      childbox.x2 -= 60;
      childbox.y2 -= 60;
      mx_allocate_align_fill (priv->welcome, &childbox, MX_ALIGN_MIDDLE,
                              MX_ALIGN_MIDDLE, TRUE, TRUE);
      clutter_actor_allocate (priv->welcome, &childbox, flags);
    }

  child_height = ((avail_box.y2 - avail_box.y1) / priv->n_children)
                  - (PADDING);
  start = avail_box.y1;

  if (!priv->is_animating)
    {
      g_list_foreach (priv->start_allocations, (GFunc) g_free, NULL);
      g_list_free (priv->start_allocations);
      priv->start_allocations = NULL;
    }

  n = 0;
  for (l = priv->children; l; l = l->next)
    {
      ClutterActor *child;
      gfloat width;

      child = CLUTTER_ACTOR (l->data);

      clutter_actor_get_preferred_width (child, child_height, NULL, &width);

      childbox.y1 = start;
      childbox.y2 = start + child_height;
      childbox.x1 = avail_box.x1 + WINDOW_EDGE_PADDING;
      childbox.x2 = (avail_box.x2 - avail_box.x1) - WINDOW_EDGE_PADDING;

      mx_allocate_align_fill (child, &childbox, MX_ALIGN_MIDDLE,
                              MX_ALIGN_MIDDLE, FALSE, FALSE);

      if (priv->is_animating)
        {
          ClutterActorBox *start, *end, now;
          gdouble alpha;

          start = g_list_nth_data (priv->start_allocations, n);
          end = &childbox;
          alpha = clutter_alpha_get_alpha (priv->alpha);

          if (!start)
            {
              /* don't know where this actor was from, so just allocate it the
               * end co-ordinates */

              clutter_actor_allocate (CLUTTER_ACTOR (l->data), end, flags);
              continue;
            }

          now.x1 = (int) (start->x1 + (end->x1 - start->x1) * alpha);
          now.x2 = (int) (start->x2 + (end->x2 - start->x2) * alpha);
          now.y1 = (int) (start->y1 + (end->y1 - start->y1) * alpha);
          now.y2 = (int) (start->y2 + (end->y2 - start->y2) * alpha);

          clutter_actor_allocate (CLUTTER_ACTOR (l->data), &now, flags);

          n++;
        }
      else
        {

          ClutterActorBox *copy = g_new (ClutterActorBox, 1);

          *copy = childbox;

          priv->start_allocations = g_list_append (priv->start_allocations,
                                                   copy);

          mx_actor_box_clamp_to_pixels (&childbox);

          clutter_actor_allocate (child, &childbox, flags);
        }

      start += child_height + PADDING;
    }
}

static void
sw_zone_paint (ClutterActor *actor)
{
  SwZonePrivate *priv = SW_ZONE (actor)->priv;
  GList *l;

  CLUTTER_ACTOR_CLASS (sw_zone_parent_class)->paint (actor);

  for (l = priv->children; l; l = l->next)
    {
      ClutterActor *child;

      child = CLUTTER_ACTOR (l->data);

      clutter_actor_paint (child);
    }

  clutter_actor_paint (priv->title);

  if (priv->dummy)
    clutter_actor_paint (priv->add_icon);

  if (priv->label && !priv->is_welcome && !priv->dummy && priv->n_children == 0)
    clutter_actor_paint (priv->label);

  if (priv->welcome && !priv->dummy && priv->is_welcome
      && priv->n_children == 0)
    clutter_actor_paint (priv->welcome);
}

static void
sw_zone_pick (ClutterActor       *actor,
              const ClutterColor *color)
{
  SwZonePrivate *priv = SW_ZONE (actor)->priv;
  GList *l;

  CLUTTER_ACTOR_CLASS (sw_zone_parent_class)->pick (actor, color);

  if (priv->drag_in_progress)
    return;

  for (l = priv->children; l; l = l->next)
    {
      ClutterActor *child;

      child = CLUTTER_ACTOR (l->data);

      if (CLUTTER_ACTOR_IS_REACTIVE (child))
        clutter_actor_paint (child);
    }
}


static void
sw_zone_map (ClutterActor *actor)
{
  SwZonePrivate *priv = SW_ZONE (actor)->priv;

  CLUTTER_ACTOR_CLASS (sw_zone_parent_class)->map (actor);

  mx_droppable_enable (MX_DROPPABLE (actor));

  if (priv->title)
    clutter_actor_map (priv->title);

  if (priv->add_icon)
    clutter_actor_map (priv->add_icon);

  if (priv->label)
    clutter_actor_map (priv->label);

  if (priv->welcome)
    clutter_actor_map (priv->welcome);
}

static void
sw_zone_unmap (ClutterActor *actor)
{
  SwZonePrivate *priv = SW_ZONE (actor)->priv;

  CLUTTER_ACTOR_CLASS (sw_zone_parent_class)->unmap (actor);

  if (priv->title)
    clutter_actor_unmap (priv->title);

  if (priv->add_icon)
    clutter_actor_unmap (priv->add_icon);

  if (priv->label)
    clutter_actor_unmap (priv->label);

  if (priv->welcome)
    clutter_actor_unmap (priv->welcome);
}


static gboolean
sw_zone_button_press_event (ClutterActor       *actor,
                            ClutterButtonEvent *event)
{
  return TRUE;
}

static void
sw_zone_class_init (SwZoneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SwZonePrivate));

  object_class->get_property = sw_zone_get_property;
  object_class->set_property = sw_zone_set_property;
  object_class->dispose = sw_zone_dispose;
  object_class->finalize = sw_zone_finalize;

  actor_class->get_preferred_width = sw_zone_get_preferred_width;
  actor_class->get_preferred_height = sw_zone_get_preferred_height;
  actor_class->allocate = sw_zone_allocate;
  actor_class->paint = sw_zone_paint;
  actor_class->pick = sw_zone_pick;
  actor_class->button_press_event = sw_zone_button_press_event;

  actor_class->map = sw_zone_map;
  actor_class->unmap = sw_zone_unmap;

  g_object_class_override_property (object_class, PROP_ENABLED, "drop-enabled");
}

static void
sw_zone_style_changed_cb (MxStylable          *zone,
                          MxStyleChangedFlags  flags)
{
  MxStylable *title = MX_STYLABLE (SW_ZONE (zone)->priv->title);
  const gchar *pseudo_class = mx_stylable_get_style_pseudo_class (zone);

  mx_stylable_set_style_pseudo_class (title, pseudo_class);

  mx_stylable_style_changed (title, flags);
}

static void
sw_zone_init (SwZone *self)
{
  self->priv = ZONE_PRIVATE (self);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  self->priv->title = mx_label_new ();
  mx_label_set_x_align (MX_LABEL (self->priv->title), MX_ALIGN_MIDDLE);
  mx_label_set_y_align (MX_LABEL (self->priv->title), MX_ALIGN_MIDDLE);
  mx_stylable_set_style_class (MX_STYLABLE (self->priv->title), "zone-title");
  clutter_actor_set_parent (self->priv->title, CLUTTER_ACTOR (self));


  self->priv->add_icon = mx_icon_new ();
  clutter_actor_set_name (self->priv->add_icon, "add-icon");
  clutter_actor_set_parent (self->priv->add_icon, CLUTTER_ACTOR (self));

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (sw_zone_style_changed_cb), NULL);

  self->priv->label = mx_label_new_with_text (_("No applications on this zone"));
  mx_stylable_set_style_class (MX_STYLABLE (self->priv->label),
                               "no-apps-label");
  clutter_actor_set_parent (self->priv->label, CLUTTER_ACTOR (self));

}

ClutterActor *
sw_zone_new (void)
{
  return g_object_new (SW_TYPE_ZONE, NULL);
}

void
sw_zone_set_dummy (SwZone   *zone,
                   gboolean  dummy)
{

  if (zone->priv->dummy == dummy)
    return;

  zone->priv->dummy = dummy;
  zone->priv->dummy_size = DUMMY_MIN_WIDTH;

  if (dummy)
    clutter_actor_set_name (CLUTTER_ACTOR (zone), "new-zone");
  else
    clutter_actor_set_name (CLUTTER_ACTOR (zone), "");

  sw_zone_update_title (zone);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (zone));
}

gboolean
sw_zone_get_dummy (SwZone *zone)
{
  return zone->priv->dummy;
}

void
sw_zone_set_focused (SwZone   *zone,
                     gboolean  focused)
{
  zone->priv->focused = focused;

  if (zone->priv->is_welcome)
    return;

  if (focused)
    clutter_actor_set_name (CLUTTER_ACTOR (zone), "focused-zone");
  else
    clutter_actor_set_name (CLUTTER_ACTOR (zone), "");
}

void
sw_zone_set_focused_window (SwZone *zone,
                            gulong  xid)
{
  GList *l;

  /* find the window and set as focused */

  for (l = zone->priv->children; l; l = g_list_next (l))
    {
      SwWindow *win = (SwWindow *) l->data;

      if (sw_window_get_xid (win) == xid)
        sw_window_set_focused (win, TRUE);
      else
        sw_window_set_focused (win, FALSE);
    }
}

gboolean
sw_zone_remove_window (SwZone *zone,
                       gulong  xid)
{
  SwZonePrivate *priv;
  GList *l;
  gboolean result = FALSE;

  /* find the window and remove */

  priv = zone->priv;

  for (l = priv->children; l; l = g_list_next (l))
    {
      SwWindow *win = (SwWindow *) l->data;

      if (sw_window_get_xid (win) == xid)
        {
          clutter_container_remove_actor (CLUTTER_CONTAINER (zone),
                                          CLUTTER_ACTOR (win));
          result = TRUE;

          break;
        }
    }

  /* remove the zone if it no longer has any windows */
  if (!priv->dummy && priv->n_children == 0)
    clutter_actor_destroy (CLUTTER_ACTOR (zone));

  /* return TRUE if a window was removed */
  return result;
}

gint
sw_zone_get_number (SwZone *zone)
{
  return zone->priv->number;
}

void
sw_zone_set_number (SwZone *zone,
                    gint    number)
{
  GList *l;

  if (zone->priv->number == number)
    return;

  zone->priv->number = number;

  for (l = zone->priv->children; l; l = g_list_next (l))
    {
      sw_window_workspace_changed (SW_WINDOW (l->data), number);
    }

  sw_zone_update_title (zone);
}

void
sw_zone_set_is_welcome (SwZone   *zone,
                        gboolean  welcome)
{
  if (zone->priv->is_welcome != welcome)
    {
      gchar *welcome_string;

      zone->priv->is_welcome = welcome;

      if (welcome)
        {
          ClutterActor *actor;
          ClutterText *text;

          zone->priv->welcome = mx_box_layout_new ();
          mx_box_layout_set_spacing (MX_BOX_LAYOUT (zone->priv->welcome), 32);

          actor = mx_label_new ();
          clutter_actor_set_name (actor, "welcome-label");

          clutter_actor_set_parent (zone->priv->welcome, CLUTTER_ACTOR (zone));

          welcome_string = _("Applications you’re using will show up here. You will be able"
                             " to switch and organize them to your heart's content.");

          text = (ClutterText*) mx_label_get_clutter_text (MX_LABEL (actor));
          clutter_text_set_markup (text, welcome_string);
          clutter_text_set_line_wrap (text, TRUE);

          mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (zone->priv->welcome),
                                                   actor, 0,
                                                   "y-align", MX_ALIGN_START,
                                                   "x-align", MX_ALIGN_MIDDLE,
                                                   NULL);

          actor = clutter_texture_new_from_file (PKGDATADIR "/people.png", NULL);

          mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (zone->priv->welcome),
                                                   actor, 1,
                                                   "y-align", MX_ALIGN_END,
                                                   "x-align", MX_ALIGN_END,
                                                   "y-fill", FALSE,
                                                   "x-fill", FALSE,
                                                   NULL);

          clutter_actor_set_name (CLUTTER_ACTOR (zone), "welcome");
          mx_label_set_text (MX_LABEL (zone->priv->title), "");
        }
      else
        {
          clutter_actor_unparent (CLUTTER_ACTOR (zone->priv->welcome));
          zone->priv->welcome = NULL;

          if (zone->priv->focused)
            clutter_actor_set_name (CLUTTER_ACTOR (zone), "focused-zone");
          else
            clutter_actor_set_name (CLUTTER_ACTOR (zone), "");

          if (zone->priv->n_children > 0)
            {
              gchar *zone_title = g_strdup_printf (_("Zone %d"),
                                                   zone->priv->number);
              mx_label_set_text (MX_LABEL (zone->priv->title), zone_title);
              g_free (zone_title);
            }
        }

      clutter_actor_queue_relayout (CLUTTER_ACTOR (zone));
    }
}

void
sw_zone_set_drag_in_progress (SwZone   *zone,
                              gboolean  drag_in_progress)
{
  zone->priv->drag_in_progress = drag_in_progress;
}
