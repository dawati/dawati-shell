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
#include <string.h>
#include <clutter/x11/clutter-x11.h>

#include "mnb-switcher-item.h"
#include "mnb-switcher-zone.h"

#define HOVER_TIMEOUT  800

struct _MnbSwitcherItemPrivate
{
  MnbSwitcher  *switcher;
  ClutterActor *tooltip;
  guint         hover_timeout_id;

  gboolean      active   : 1;
  gboolean      disposed : 1;
};

enum
{
  PROP_0 = 0,

  PROP_SWITCHER,
};

G_DEFINE_ABSTRACT_TYPE (MnbSwitcherItem, mnb_switcher_item, MX_TYPE_FRAME);

#define MNB_SWITCHER_ITEM_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER_ITEM,\
                                MnbSwitcherItemPrivate))

static void
mnb_switcher_item_dispose (GObject *object)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->hover_timeout_id)
    {
      g_source_remove (priv->hover_timeout_id);
      priv->hover_timeout_id = 0;
    }

  if (priv->tooltip)
    {
      ClutterActor *parent = clutter_actor_get_parent (priv->tooltip);

      clutter_container_remove_actor (CLUTTER_CONTAINER (parent),
                                      priv->tooltip);
      priv->tooltip = NULL;
    }

  G_OBJECT_CLASS (mnb_switcher_item_parent_class)->dispose (object);
}

static void
mnb_switcher_item_allocate (ClutterActor          *actor,
                            const ClutterActorBox *box,
                            ClutterAllocationFlags flags)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (actor)->priv;
  ClutterGeometry         area;
  gfloat                  x, y, w, h;

  /*
   * Let the parent class do it's thing, and then allocate for the tooltip.
   */
  CLUTTER_ACTOR_CLASS (mnb_switcher_item_parent_class)->allocate (actor,
                                                                  box, flags);

  /* set the tooltip area */
  clutter_actor_get_transformed_position (actor, &x, &y);
  clutter_actor_get_size (actor, &w, &h);

  area.x      = x;
  area.y      = y;
  area.width  = w;
  area.height = h;

  mx_tooltip_set_tip_area ((MxTooltip*) priv->tooltip, &area);
}

static void
mnb_switcher_item_set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SWITCHER:
      priv->switcher = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_item_get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SWITCHER:
      g_value_set_object (value, priv->switcher);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

/*
 * The callback to hide tooltip after predefined period of time.
 */
static gboolean
mnb_switcher_item_hover_timeout_cb (gpointer data)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (data)->priv;

  if (!mnb_switcher_get_dnd_in_progress (priv->switcher))
    {
      mnb_switcher_show_tooltip (priv->switcher, MX_TOOLTIP (priv->tooltip));
    }

  priv->hover_timeout_id = 0;

  return FALSE;
}

/*
 * ClutterActor::enter_event vfunction implementation
 * Trigger showing tooltip.
 */
static gboolean
mnb_switcher_item_enter_event (ClutterActor *actor, ClutterCrossingEvent *event)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (actor)->priv;

  if (!mnb_switcher_get_dnd_in_progress (priv->switcher))
    priv->hover_timeout_id = g_timeout_add (HOVER_TIMEOUT,
                                            mnb_switcher_item_hover_timeout_cb,
                                            actor);

  return FALSE;
}

/*
 * ClutterActor::enter_event vfunction implementation
 * Hide tooltip.
 */
static gboolean
mnb_switcher_item_leave_event (ClutterActor *actor, ClutterCrossingEvent *event)
{
  MnbSwitcherItemPrivate *priv = MNB_SWITCHER_ITEM (actor)->priv;

  if (priv->hover_timeout_id)
    {
      g_source_remove (priv->hover_timeout_id);
      priv->hover_timeout_id = 0;
    }

  if (CLUTTER_ACTOR_IS_MAPPED (priv->tooltip))
    {
      mnb_switcher_hide_tooltip (priv->switcher);
    }

  return FALSE;
}

static void
mnb_switcher_item_constructed (GObject *self)
{
  ClutterActor           *actor = CLUTTER_ACTOR (self);
  MnbSwitcherItemPrivate *priv  = MNB_SWITCHER_ITEM (self)->priv;

  priv->tooltip  = g_object_new (MX_TYPE_TOOLTIP, NULL);
  clutter_actor_set_parent (priv->tooltip, actor);
}

static void
mnb_switcher_item_class_init (MnbSwitcherItemClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class    = CLUTTER_ACTOR_CLASS (klass);

  object_class->dispose             = mnb_switcher_item_dispose;
  object_class->get_property        = mnb_switcher_item_get_property;
  object_class->set_property        = mnb_switcher_item_set_property;
  object_class->constructed         = mnb_switcher_item_constructed;

  actor_class->allocate             = mnb_switcher_item_allocate;
  actor_class->enter_event          = mnb_switcher_item_enter_event;
  actor_class->leave_event          = mnb_switcher_item_leave_event;

  g_type_class_add_private (klass, sizeof (MnbSwitcherItemPrivate));

  g_object_class_install_property (object_class,
                                   PROP_SWITCHER,
                                   g_param_spec_object ("switcher",
                                                      "Switcher",
                                                      "Switcher",
                                                      MNB_TYPE_SWITCHER,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

}

static void
mnb_switcher_item_init (MnbSwitcherItem *self)
{
  MnbSwitcherItemPrivate *priv;

  priv = self->priv = MNB_SWITCHER_ITEM_GET_PRIVATE (self);
}

void
mnb_switcher_item_show_tooltip (MnbSwitcherItem *self)
{
  MnbSwitcherItemPrivate *priv = self->priv;
  ClutterGeometry         area;
  gfloat                  x, y, w, h;
  ClutterActor           *actor = CLUTTER_ACTOR (self);

  if (!priv->tooltip)
    return;

  /*
   * Ensure the tooltip is in the correct position (MB#7225)
   */
  clutter_actor_get_transformed_position (actor, &x, &y);
  clutter_actor_get_size (actor, &w, &h);

  area.x      = x;
  area.y      = y;
  area.width  = w;
  area.height = h;

  mx_tooltip_set_tip_area ((MxTooltip*) priv->tooltip, &area);

  mnb_switcher_show_tooltip (priv->switcher, MX_TOOLTIP (priv->tooltip));
}

void
mnb_switcher_item_hide_tooltip (MnbSwitcherItem *self)
{
  MnbSwitcherItemPrivate *priv = self->priv;

  if (!priv->tooltip)
    return;

  if (priv->hover_timeout_id)
    {
      g_source_remove (priv->hover_timeout_id);
      priv->hover_timeout_id = 0;
    }

  mnb_switcher_hide_tooltip (priv->switcher);
}

/*
 * Put this item into active state, e.g., apply apropriate css style.
 */
void
mnb_switcher_item_set_active (MnbSwitcherItem *self, gboolean active)
{
  MnbSwitcherItemPrivate *priv        = self->priv;
  MnbSwitcherItemClass   *klass       = MNB_SWITCHER_ITEM_GET_CLASS (self);
  const gchar            *active_name = NULL;

  if ((active && priv->active) || (!active && !priv->active))
    return;

  priv->active = active;

  if (klass->active_style)
    active_name = klass->active_style  (self);

  if (!active_name)
    return;

  if (active)
    clutter_actor_set_name (CLUTTER_ACTOR (self), active_name);
  else
    clutter_actor_set_name (CLUTTER_ACTOR (self),"");
}

gboolean
mnb_switcher_item_is_active (MnbSwitcherItem *self)
{
  MnbSwitcherItemPrivate *priv = self->priv;

  return priv->active;
}

/*
 * Sets the tooltip text for this item.
 */
void
mnb_switcher_item_set_tooltip (MnbSwitcherItem *self, const gchar *text)
{
  MnbSwitcherItemPrivate *priv = self->priv;

  mx_tooltip_set_label (MX_TOOLTIP (priv->tooltip), text);
}

MnbSwitcher *
mnb_switcher_item_get_switcher (MnbSwitcherItem *self)
{
  MnbSwitcherItemPrivate *priv = self->priv;

  return priv->switcher;
}

/*
 * Gets the zone object this item resides on.
 */
MnbSwitcherZone*
mnb_switcher_item_get_zone (MnbSwitcherItem *item)
{
  ClutterActor *parent = clutter_actor_get_parent (CLUTTER_ACTOR (item));

  while (parent && !MNB_IS_SWITCHER_ZONE (parent))
    parent = clutter_actor_get_parent (parent);

  return (MnbSwitcherZone*)parent;
}

/*
 * Activate this item (whatever that might mean for any particular subclass)
 */
gboolean
mnb_switcher_item_activate (MnbSwitcherItem *item)
{
  MnbSwitcherItemClass *klass = MNB_SWITCHER_ITEM_GET_CLASS (item);

  if (klass->activate)
    return klass->activate (item);

  return FALSE;
}
