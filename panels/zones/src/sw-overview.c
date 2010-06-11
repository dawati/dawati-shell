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

#include "sw-overview.h"
#include "sw-zone.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (SwOverview, sw_overview, MX_TYPE_BOX_LAYOUT)

#define OVERVIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SW_TYPE_OVERVIEW, SwOverviewPrivate))

enum
{
  PROP_N_ZONES = 1
};

typedef struct
{
  SwWindow *window;
  gulong handler;
} WindowNotify;

struct _SwOverviewPrivate
{
  gint n_zones;
  gint focused_zone;

  ClutterActor *dummy;
  gulong dummy_added_handler;
  gulong zone_removed_handler;

  gint window_count;
  GList *window_handlers;
};


static void
sw_overview_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  SwOverviewPrivate *priv = SW_OVERVIEW (object)->priv;

  switch (property_id)
    {
  case PROP_N_ZONES:
    g_value_set_int (value, priv->n_zones);
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sw_overview_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{

  SwOverviewPrivate *priv = SW_OVERVIEW (object)->priv;

  switch (property_id)
    {
  case PROP_N_ZONES:
    priv->n_zones = g_value_get_int (value);
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sw_overview_dispose (GObject *object)
{
  SwOverviewPrivate *priv = SW_OVERVIEW (object)->priv;
  GList *l;

  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (object), FALSE);

  if (priv->window_handlers)
    {
      for (l = priv->window_handlers; l; l = g_list_next (l))
        {
          WindowNotify *notify = (WindowNotify *) l->data;

          g_signal_handler_disconnect (notify->window, notify->handler);

          g_free (notify);
        }

      g_list_free (priv->window_handlers);
      priv->window_handlers = NULL;
    }

  if (priv->zone_removed_handler)
    {
      g_signal_handler_disconnect (object, priv->zone_removed_handler);
      priv->zone_removed_handler = 0;
    }

  if (priv->dummy)
    {
      /* this is a pointer to a particular child, so it is not necessary to
       * unref it here */
      priv->dummy = NULL;
    }

  G_OBJECT_CLASS (sw_overview_parent_class)->dispose (object);
}

static void actor_added_to_dummy (ClutterContainer *zone, ClutterActor *actor, SwOverview *view);

static void sw_overview_renumber_zones (SwOverview *view);

static void
sw_overview_finalize (GObject *object)
{
  G_OBJECT_CLASS (sw_overview_parent_class)->finalize (object);
}

static void
sw_overview_add_dummy (SwOverview *view)
{
  SwOverviewPrivate *priv = view->priv;

  priv->dummy = sw_zone_new ();
  sw_zone_set_dummy (SW_ZONE (priv->dummy), TRUE);
  priv->dummy_added_handler = g_signal_connect (priv->dummy, "actor-added",
                                                G_CALLBACK (actor_added_to_dummy),
                                                view);

  clutter_container_add_actor (CLUTTER_CONTAINER (view), priv->dummy);
}

static void
actor_added_to_dummy (ClutterContainer *zone,
                      ClutterActor     *actor,
                      SwOverview       *view)
{
  SwOverviewPrivate *priv = view->priv;

  /* disconnect this handler since it is now a real zone */
  g_signal_handler_disconnect (zone, priv->dummy_added_handler);
  sw_zone_set_dummy (SW_ZONE (zone), FALSE);
  clutter_container_child_set (CLUTTER_CONTAINER (view),
                               CLUTTER_ACTOR (zone), "expand", TRUE, NULL);

  priv->dummy = NULL;

  priv->n_zones++;
  g_debug ("New zone added (count: %d)", priv->n_zones);

  sw_overview_renumber_zones (view);
}

static void
sw_overview_renumber_zones (SwOverview   *view)
{
  GList *children, *l;
  gint count;

  children = clutter_container_get_children (CLUTTER_CONTAINER (view));

  count = 0;
  for (l = children; l; l = g_list_next (l))
    {
      SwZone *zone = SW_ZONE (l->data);

      if (!sw_zone_get_dummy (zone))
        {
          sw_zone_set_number (zone, ++count);

          if (count == view->priv->focused_zone)
            sw_zone_set_focused (zone, TRUE);
          else
            sw_zone_set_focused (zone, FALSE);
        }
    }

  /* ensure there is at least one zone at all times */
  if (count == 0)
    {
      ClutterActor *zone;

      /* temporarily disable animations to add to the welcome screen */
      mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (view), FALSE);

      zone = sw_zone_new ();
      clutter_container_add_actor (CLUTTER_CONTAINER (view), zone);
      clutter_container_child_set (CLUTTER_CONTAINER (view),
                                   zone, "expand", TRUE, NULL);
      sw_zone_set_is_welcome (SW_ZONE (zone), TRUE);
      sw_zone_set_number (SW_ZONE (zone), 1);

      /* re-enable animations after welcome screen is added */
      mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (view), TRUE);

      view->priv->n_zones++;
    }

  /* add the new dummy */
  if (count < 8 && !view->priv->dummy && count < view->priv->window_count)
    sw_overview_add_dummy (view);

}

static void
sw_overview_constructed (GObject *object)
{
  SwOverviewPrivate *priv = SW_OVERVIEW (object)->priv;
  gint i;
  ClutterActor *zone;

  g_debug ("Creating a zones panel with %d zones", priv->n_zones);

  for (i = 0; i < priv->n_zones; i++)
    {
      zone = sw_zone_new ();
      clutter_container_add_actor (CLUTTER_CONTAINER (object), zone);
      clutter_container_child_set (CLUTTER_CONTAINER (object),
                                   zone, "expand", TRUE, NULL);
    }

  if (priv->n_zones == 1)
    {
      sw_zone_set_is_welcome (SW_ZONE (zone), TRUE);
    }

  sw_overview_renumber_zones (SW_OVERVIEW (object));
}

static void
sw_overview_class_init (SwOverviewClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SwOverviewPrivate));

  object_class->get_property = sw_overview_get_property;
  object_class->set_property = sw_overview_set_property;
  object_class->dispose = sw_overview_dispose;
  object_class->finalize = sw_overview_finalize;
  object_class->constructed = sw_overview_constructed;

  pspec = g_param_spec_int ("n-zones",
                            "N Zones",
                            "Number of Zones",
                            0, G_MAXINT, 0,
                            G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_N_ZONES, pspec);

}

void
sw_overview_add_zone (SwOverview *self)
{
  ClutterActor *zone;
  SwOverviewPrivate *priv = self->priv;

  if (priv->dummy)
    clutter_container_remove_actor (CLUTTER_CONTAINER (self), priv->dummy);
  priv->dummy = NULL;

  zone = sw_zone_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), zone);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               zone, "expand", TRUE, NULL);

  sw_overview_renumber_zones (self);

  priv->n_zones++;
  g_debug ("Added a zone (count: %d)", priv->n_zones);
}

static void
zone_removed (SwOverview *overview,
              SwZone     *zone)
{
  if (!sw_zone_get_dummy (zone))
    {
      overview->priv->n_zones--;
      g_debug ("Zone removed (count: %d)", overview->priv->n_zones);
      sw_overview_renumber_zones (overview);
    }
}

static void
sw_overview_init (SwOverview *self)
{
  self->priv = OVERVIEW_PRIVATE (self);

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 9);

  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (self), TRUE);

  self->priv->zone_removed_handler = g_signal_connect (self, "actor-removed",
                                                      G_CALLBACK (zone_removed),
                                                      NULL);
}

ClutterActor *
sw_overview_new (gint n_zones)
{
  return g_object_new (SW_TYPE_OVERVIEW, "n-zones", n_zones, NULL);
}


void
window_drag_begin (SwOverview *overview)
{
  GList *children, *l;

  children = clutter_container_get_children (CLUTTER_CONTAINER (overview));

  for (l = children; l; l = g_list_next (l))
    {
      sw_zone_set_drag_in_progress (SW_ZONE (l->data), TRUE);
    }

  g_list_free (children);
}

void
window_drag_end (SwOverview *overview)
{
  GList *children, *l;

  children = clutter_container_get_children (CLUTTER_CONTAINER (overview));

  for (l = children; l; l = g_list_next (l))
    {
      sw_zone_set_drag_in_progress (SW_ZONE (l->data), FALSE);
    }

  g_list_free (children);
}


static void
window_removed (SwWindow   *window,
                SwOverview *overview)
{
  GList *l;
  SwOverviewPrivate *priv = overview->priv;

  priv->window_count--;

  if (priv->window_count <= priv->n_zones)
    {
      if (priv->dummy)
        clutter_actor_destroy (priv->dummy);
      priv->dummy = NULL;
    }

  /* find the stored handler id and remove it */
  for (l = priv->window_handlers; l; l = g_list_next (l))
    {
      WindowNotify *notify = (WindowNotify*) l->data;

      if (notify->window == window)
        {
          g_free (notify);

          priv->window_handlers = g_list_delete_link (priv->window_handlers, l);

          break;
        }
    }
}

void
sw_overview_add_window (SwOverview *overview,
                        SwWindow   *window,
                        gint        index)
{
  SwOverviewPrivate *priv = overview->priv;
  GList *children;
  ClutterContainer *zone;
  WindowNotify *notify;

  children = clutter_container_get_children (CLUTTER_CONTAINER (overview));

  zone = g_list_nth_data (children, index);

  if (!zone)
    {
      g_warning ("zones-panel: adding a window to non-existant zone (%d)", index);
      return;
    }

  clutter_container_add_actor (zone, CLUTTER_ACTOR (window));


  g_signal_connect_swapped (window, "drag-begin",
                            G_CALLBACK (window_drag_begin), overview);
  g_signal_connect_swapped (window, "drag-end",
                            G_CALLBACK (window_drag_end), overview);

  /* increase window count */
  priv->window_count++;

  if (priv->window_count == 1)
    {
      GList *l;
      /* first window is added, ensure there is no welcome screen */
      for (l = children; l; l = l->next)
        sw_zone_set_is_welcome (SW_ZONE (l->data), FALSE);
    }

  /* decrease window count when the window is deleted */
  notify = g_new (WindowNotify, 1);
  notify->window = window;
  notify->handler = g_signal_connect (window, "destroy",
                                      G_CALLBACK (window_removed), overview);
  priv->window_handlers = g_list_prepend (priv->window_handlers, notify);


  /* ensure there is no "new zone" if the number of windows is less than or
   * equal to the zones */
  if (priv->window_count > priv->n_zones
      && !priv->dummy
      && priv->n_zones < 8)
    {
      sw_overview_add_dummy (overview);
    }
  else
    {
      if (priv->window_count <= priv->n_zones && priv->dummy)
        {
          clutter_actor_destroy (priv->dummy);
          priv->dummy = NULL;
        }
    }

  g_debug ("Window added (windows: %d; zones: %d)", priv->window_count,
           priv->n_zones);

  g_list_free (children);
}

void
sw_overview_remove_window (SwOverview *overview,
                           gulong      xid)
{
  GList *children, *l;

  children = clutter_container_get_children (CLUTTER_CONTAINER (overview));

  for (l = children; l; l = g_list_next (l))
    {
      if (sw_zone_remove_window (SW_ZONE (l->data), xid))
        break;
    }

  g_list_free (children);
}

void
sw_overview_set_focused_zone (SwOverview *overview,
                              gint        index)
{
  GList *children, *l;
  guint i;

  children = clutter_container_get_children (CLUTTER_CONTAINER (overview));

  i = 0;
  for (l = children; l; l = g_list_next (l))
    {
      sw_zone_set_focused (SW_ZONE (l->data), (i == index));
      i++;
    }

  overview->priv->focused_zone = index + 1;

  g_list_free (children);
}

void
sw_overview_set_focused_window (SwOverview *overview,
                                gulong      xid)
{
  GList *children, *l;

  children = clutter_container_get_children (CLUTTER_CONTAINER (overview));

  for (l = children; l; l = g_list_next (l))
    {
      sw_zone_set_focused_window (SW_ZONE (l->data), xid);
    }

  g_list_free (children);
}

