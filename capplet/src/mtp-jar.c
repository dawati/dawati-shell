/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-jar-button.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#include <config.h>
#include <glib/gi18n-lib.h>

#include "mtp-jar.h"
#include "mtp-toolbar-button.h"
#include "mtp-space.h"

static void mx_droppable_iface_init (MxDroppableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MtpJar, mtp_jar, MX_TYPE_BOX_LAYOUT,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DROPPABLE,
                                                mx_droppable_iface_init));

#define MTP_JAR_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_JAR, MtpJarPrivate))

enum
{
  ZONE_PROP_0 = 0,

  /* d&d */
  ZONE_PROP_ENABLED
};

struct _MtpJarPrivate
{
  ClutterActor *panel_area;
  ClutterActor *applet_area;
  ClutterActor *label;
  ClutterActor *space;

  gboolean enabled  : 1;
  gboolean disposed : 1;
};

static void
mtp_jar_dispose (GObject *object)
{
  MtpJarPrivate *priv = MTP_JAR (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  g_object_unref (priv->space);

  G_OBJECT_CLASS (mtp_jar_parent_class)->dispose (object);
}

static void
mtp_jar_map (ClutterActor *actor)
{
  /* MtpJarPrivate *priv = MTP_JAR (actor)->priv; */

  CLUTTER_ACTOR_CLASS (mtp_jar_parent_class)->map (actor);
}

static void
mtp_jar_unmap (ClutterActor *actor)
{
  /* MtpJarPrivate *priv = MTP_JAR (actor)->priv; */

  CLUTTER_ACTOR_CLASS (mtp_jar_parent_class)->unmap (actor);
}

static void
mtp_jar_allocate (ClutterActor          *actor,
                      const ClutterActorBox *box,
                      ClutterAllocationFlags flags)
{
  /* MtpJarPrivate *priv = MTP_JAR (actor)->priv; */

  CLUTTER_ACTOR_CLASS (mtp_jar_parent_class)->allocate (actor, box, flags);
}

static void
mtp_jar_constructed (GObject *self)
{
  MtpJarPrivate *priv = MTP_JAR (self)->priv;
  ClutterActor  *scroll1, *scroll2;
  ClutterActor  *bin;

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 10);

  priv->label = mx_label_new_with_text (_("Available panels"));
  clutter_actor_set_name (priv->label, "available-panels");

  bin = mx_frame_new ();
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  clutter_actor_set_name (bin, "content-header");
  clutter_container_add_actor (CLUTTER_CONTAINER (bin), priv->label);

  priv->panel_area  = mx_grid_new ();
  clutter_actor_set_name (priv->panel_area, "jar-panel-area");
  mx_grid_set_column_spacing (MX_GRID (priv->panel_area), 2);
  mx_grid_set_row_spacing (MX_GRID (priv->panel_area), 2);
  scroll1 = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll1), priv->panel_area);

  priv->applet_area  = mx_grid_new ();
  clutter_actor_set_name (priv->panel_area, "jar-applet-area");
  mx_grid_set_column_spacing (MX_GRID (priv->applet_area), 2);
  mx_grid_set_row_spacing (MX_GRID (priv->applet_area), 2);
  scroll2 = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll2), priv->applet_area);

  clutter_container_add (CLUTTER_CONTAINER (self),
                         bin, scroll1, scroll2, NULL);

  priv->space = mtp_space_new ();
  g_object_ref_sink (priv->space);

}

static void
mtp_jar_set_property (GObject      *gobject,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  MtpJarPrivate *priv = MTP_JAR (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      priv->enabled = g_value_get_boolean (value);
      if (priv->enabled)
        mx_droppable_enable (MX_DROPPABLE (gobject));
      else
        mx_droppable_disable (MX_DROPPABLE (gobject));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mtp_jar_get_property (GObject    *gobject,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  MtpJarPrivate *priv = MTP_JAR (gobject)->priv;

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

static void
mtp_jar_over_in (MxDroppable *droppable, MxDraggable *draggable)
{
  MtpJarPrivate *priv = MTP_JAR (droppable)->priv;
  MtpToolbarButton *button = MTP_TOOLBAR_BUTTON (draggable);
  ClutterActor *parent;

  if (!priv->enabled)
    return;

  parent = clutter_actor_get_parent (priv->space);

  if (parent)
    {
      g_warning (G_STRLOC " Should not have parent here ...");
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), priv->space);
    }

  if (mtp_toolbar_button_is_applet (button))
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->applet_area),
                                 priv->space);
  else
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                 priv->space);
}

static void
mtp_jar_over_out (MxDroppable *droppable, MxDraggable *draggable)
{
  MtpJarPrivate *priv = MTP_JAR (droppable)->priv;
  ClutterActor  *parent;

  if (!priv->enabled)
    return;

  parent = clutter_actor_get_parent (priv->space);

  if (parent)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), priv->space);
    }
}

/*
 * D&D drop handler
 */
static void
mtp_jar_drop (MxDroppable       *droppable,
              MxDraggable       *draggable,
              gfloat               event_x,
              gfloat               event_y,
              gint                 button,
              ClutterModifierType  modifiers)
{
  MtpJar            *jar = MTP_JAR (droppable);
  MtpJarPrivate     *priv = jar->priv;
  ClutterActor      *actor = CLUTTER_ACTOR (draggable);
  ClutterActor  *parent;

  /*
   * Check we are not disabled (we should really not be getting drop events on
   * disabled droppables, but we do).
   */
  if (!priv->enabled)
    {
      g_warning ("Bug: received a drop on a disabled droppable -- ignoring");
      return;
    }

  parent = clutter_actor_get_parent (priv->space);

  if (parent)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), priv->space);
    }

  clutter_actor_set_size (actor, -1.0, -1.0);
  mtp_jar_add_button (jar, actor);
}

static void
mx_droppable_iface_init (MxDroppableIface *iface)
{
  iface->over_in  = mtp_jar_over_in;
  iface->over_out = mtp_jar_over_out;
  iface->drop     = mtp_jar_drop;
}

static void
mtp_jar_class_init (MtpJarClass *klass)
{
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MtpJarPrivate));

  actor_class->allocate           = mtp_jar_allocate;
  actor_class->map                = mtp_jar_map;
  actor_class->unmap              = mtp_jar_unmap;

  object_class->constructed       = mtp_jar_constructed;
  object_class->dispose           = mtp_jar_dispose;
  object_class->set_property      = mtp_jar_set_property;
  object_class->get_property      = mtp_jar_get_property;

  g_object_class_override_property (object_class, ZONE_PROP_ENABLED,
                                    "drop-enabled");
}

static void
mtp_jar_init (MtpJar *self)
{
  self->priv = MTP_JAR_GET_PRIVATE (self);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);
  clutter_actor_set_reactive ((ClutterActor*)self, TRUE);
}

ClutterActor*
mtp_jar_new (void)
{
  return g_object_new (MTP_TYPE_JAR, NULL);
}

void
mtp_jar_add_button (MtpJar *jar, ClutterActor *button)
{
  MtpJarPrivate *priv = MTP_JAR (jar)->priv;
  gboolean       has_parent;

  if (!MTP_IS_TOOLBAR_BUTTON (button))
    {
      g_warning (G_STRLOC ":%s: unsupported button type %s",
                 __FUNCTION__, G_OBJECT_TYPE_NAME (button));
      return;
    }

  has_parent = (clutter_actor_get_parent (button) != NULL);

  if (mtp_toolbar_button_is_applet ((MtpToolbarButton*)button))
    {
      if (has_parent)
        clutter_actor_reparent (button, priv->applet_area);
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->applet_area),
                                     CLUTTER_ACTOR (button));
    }
  else
    {
      if (has_parent)
        clutter_actor_reparent (button, priv->panel_area);
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                     CLUTTER_ACTOR (button));
    }
}

void
mtp_jar_remove_button (MtpJar *jar, ClutterActor *button)
{
  MtpJarPrivate *priv = MTP_JAR (jar)->priv;

  if (!MTP_IS_TOOLBAR_BUTTON (button))
    {
      g_warning (G_STRLOC ":%s: unsupported button type %s",
                 __FUNCTION__, G_OBJECT_TYPE_NAME (button));
      return;
    }

  if (mtp_toolbar_button_is_applet ((MtpToolbarButton*)button))
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->applet_area),
                                    CLUTTER_ACTOR (button));
  else
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->panel_area),
                                    CLUTTER_ACTOR (button));
}

