/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
 */

#include <glib/gi18n.h>
#include <clutter/x11/clutter-x11.h>

#include "mnb-switcher-zone-new.h"
#include "mnb-switcher-app.h"

struct _MnbSwitcherZoneNewPrivate
{
  /* d&d */
  gboolean enabled  : 1;
};

enum
{
  ZONE_PROP_0 = 0,
  ZONE_PROP_ENABLED
};

static void nbtk_droppable_iface_init (NbtkDroppableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbSwitcherZoneNew, mnb_switcher_zone_new,
                         MNB_TYPE_SWITCHER_ZONE,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_DROPPABLE,
                                                nbtk_droppable_iface_init));

#define MNB_SWITCHER_ZONE_NEW_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER_ZONE_NEW,\
                                MnbSwitcherZoneNewPrivate))

static void
mnb_switcher_zone_new_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_switcher_zone_new_parent_class)->dispose (object);
}

static void
mnb_switcher_zone_new_get_preferred_width (ClutterActor *actor,
                                           gfloat for_height,
                                           gfloat *min_width,
                                           gfloat *pref_width)
{
  /* This is is a nasty hack to ignore the size of the children.
   * It allows the parent to allocate each of the zones equally, because they
   * all report the same width */

  if (min_width)
    *min_width = 22.0;

  if (pref_width)
    *pref_width = 22.0;
}

static void
mnb_switcher_zone_new_over_in (NbtkDroppable *droppable,
                               NbtkDraggable *draggable)
{
  MnbSwitcherZone           *zone = MNB_SWITCHER_ZONE (droppable);
  MnbSwitcherZoneNewPrivate *priv = MNB_SWITCHER_ZONE_NEW (droppable)->priv;

  if (!priv->enabled)
    return;

  mnb_switcher_zone_set_state (zone, MNB_SWITCHER_ZONE_HOVER);
}

static void
mnb_switcher_zone_new_over_out (NbtkDroppable *droppable,
                                NbtkDraggable *draggable)
{
  MnbSwitcherZone           *zone = MNB_SWITCHER_ZONE (droppable);
  MnbSwitcherZoneNewPrivate *priv = MNB_SWITCHER_ZONE_NEW (droppable)->priv;

  if (!priv->enabled)
    return;

  mnb_switcher_zone_reset_state (zone);
}

/*
 * The drop handler; we ask the switcher to create a new zone, then move the
 * item onto the new zone.
 */
static void
mnb_switcher_zone_new_drop (NbtkDroppable       *droppable,
                            NbtkDraggable       *draggable,
                            gfloat               event_x,
                            gfloat               event_y,
                            gint                 button,
                            ClutterModifierType  modifiers)
{
  MnbSwitcherZone            *zone = MNB_SWITCHER_ZONE (droppable);
  MnbSwitcherZoneNewPrivate  *priv = MNB_SWITCHER_ZONE_NEW (droppable)->priv;
  MnbSwitcherApp             *app  = MNB_SWITCHER_APP (draggable);
  ClutterActor               *zone_actor = CLUTTER_ACTOR (droppable);
  ClutterActor               *app_actor = CLUTTER_ACTOR (draggable);
  ClutterActor               *parent;
  MutterWindow               *mcw;
  MetaWindow                 *meta_win;
  guint32                     timestamp;
  MnbSwitcher                *switcher;
  ClutterActor               *new_zone;
  gint                        index;
  NbtkTable                  *content_area;

  mcw      = mnb_switcher_app_get_window (app);
  meta_win = mutter_window_get_meta_window (mcw);

  if (!meta_win)
    {
      g_warning ("No MutterWindow associated with this item.");
      return;
    }

  /*
   * Check we are not disabled (we should really not be getting drop events on
   * disabled droppables, but we do).
   */
  if (!priv->enabled)
    {
      g_warning ("Bug: received a drop on a disabled droppable -- ignoring");
      return;
    }

  /*
   * Check whether we are not being dropped back on the same zone
   */
  if (mnb_switcher_app_get_pre_drag_parent (app) == zone_actor)
    {
      /*
       * Do nothing here; this is equivalent to the d&d being cancelled and is
       * handled in the drag_end () function.
       */
      return;
    }

  switcher = mnb_switcher_zone_get_switcher (zone);
  index    = mnb_switcher_zone_get_index (zone);
  parent   = clutter_actor_get_parent (app_actor);
  new_zone = mnb_switcher_append_app_zone (switcher, index);
  content_area = mnb_switcher_zone_get_content_area (MNB_SWITCHER_ZONE (new_zone));

  g_object_ref (draggable);

  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), app_actor);
  clutter_actor_set_size (app_actor, -1.0, -1.0);
  nbtk_table_add_actor (content_area, app_actor, 0, 0);
  clutter_container_child_set (CLUTTER_CONTAINER (content_area), app_actor,
                               "y-fill", FALSE, "x-fill", FALSE,  NULL);

  if (mnb_switcher_item_is_active (MNB_SWITCHER_ITEM (app)))
    mnb_switcher_zone_set_active (MNB_SWITCHER_ZONE (new_zone), TRUE);

  g_object_unref (draggable);

  timestamp = clutter_x11_get_current_event_time ();
  meta_window_change_workspace_by_index (meta_win,
                                         mnb_switcher_zone_get_index (zone),
                                         TRUE, timestamp);
}

static void
nbtk_droppable_iface_init (NbtkDroppableIface *iface)
{
  iface->over_in  = mnb_switcher_zone_new_over_in;
  iface->over_out = mnb_switcher_zone_new_over_out;
  iface->drop     = mnb_switcher_zone_new_drop;
}

static void
mnb_switcher_zone_new_set_property (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MnbSwitcherZoneNewPrivate *priv = MNB_SWITCHER_ZONE_NEW (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      priv->enabled = g_value_get_boolean (value);
      if (priv->enabled)
        nbtk_droppable_enable (NBTK_DROPPABLE (gobject));
      else
        nbtk_droppable_disable (NBTK_DROPPABLE (gobject));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_zone_new_get_property (GObject    *gobject,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  MnbSwitcherZoneNewPrivate *priv = MNB_SWITCHER_ZONE_NEW (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      g_value_set_boolean (value, priv->enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

/*
 * Implementation of MnbSwitcherZone::zone_style vfunction
 */
static const gchar *
mnb_switcher_zone_new_zone_style (MnbSwitcherZone      *zone,
                                  MnbSwitcherZoneState  state)
{
  const gchar *zone_name;

  switch (state)
    {
    default:
    case MNB_SWITCHER_ZONE_NORMAL:
      zone_name = "";
      break;
    case MNB_SWITCHER_ZONE_ACTIVE:
      zone_name = "switcher-workspace-new-active";
      break;
    case MNB_SWITCHER_ZONE_HOVER:
      zone_name = "switcher-workspace-new-over";
      break;
    }

  return zone_name;
}

/*
 * Implementation of MnbSwitcherZone::label_style vfunction
 */
static const gchar *
mnb_switcher_zone_new_label_style (MnbSwitcherZone     *zone,
                                   MnbSwitcherZoneState state)
{
  const gchar *label_name;

  switch (state)
    {
    default:
    case MNB_SWITCHER_ZONE_NORMAL:
      label_name = "";
      break;
    case MNB_SWITCHER_ZONE_ACTIVE:
      label_name = "workspace-title-new-active";
      break;
    case MNB_SWITCHER_ZONE_HOVER:
      label_name = "workspace-title-new-over";
      break;
    }

  return label_name;
}

static void
mnb_switcher_zone_new_constructed (GObject *self)
{
  if (G_OBJECT_CLASS (mnb_switcher_zone_new_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_switcher_zone_new_parent_class)->constructed (self);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
}

/*
 * Implementation of MnbSwitcherZone::zone_class vfunction
 */
const gchar *
mnb_switcher_zone_new_zone_class (MnbSwitcherZone *zone)
{
  return "switcher-workspace-new";
}

/*
 * Implementation of MnbSwitcherZone::label_class vfunction
 */
const gchar *
mnb_switcher_zone_new_label_class (MnbSwitcherZone *zone)
{
  return "workspace-title-new";
}

static void
mnb_switcher_zone_new_class_init (MnbSwitcherZoneNewClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass    *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  MnbSwitcherZoneClass *zone_class   = MNB_SWITCHER_ZONE_CLASS (klass);

  object_class->dispose            = mnb_switcher_zone_new_dispose;
  object_class->constructed        = mnb_switcher_zone_new_constructed;
  object_class->get_property       = mnb_switcher_zone_new_get_property;
  object_class->set_property       = mnb_switcher_zone_new_set_property;

  actor_class->get_preferred_width = mnb_switcher_zone_new_get_preferred_width;

  zone_class->zone_style           = mnb_switcher_zone_new_zone_style;
  zone_class->label_style          = mnb_switcher_zone_new_label_style;
  zone_class->zone_class           = mnb_switcher_zone_new_zone_class;
  zone_class->label_class          = mnb_switcher_zone_new_label_class;

  g_type_class_add_private (klass, sizeof (MnbSwitcherZoneNewPrivate));

  g_object_class_override_property (object_class, ZONE_PROP_ENABLED, "enabled");
}

static void
mnb_switcher_zone_new_init (MnbSwitcherZoneNew *self)
{
  self->priv = MNB_SWITCHER_ZONE_NEW_GET_PRIVATE (self);
}

MnbSwitcherZone *
mnb_switcher_zone_new_new (MnbSwitcher *switcher, gint index)
{
  return g_object_new (MNB_TYPE_SWITCHER_ZONE_NEW,
                       "switcher",   switcher,
                       "index",      index,
                       "label-text", "",
                       NULL);
}

