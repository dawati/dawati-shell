
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include <stdbool.h>

#include "mpl-content-pane.h"

static void
_container_interface_init (ClutterContainerIface *interface);

G_DEFINE_TYPE_WITH_CODE (MplContentPane, mpl_content_pane, MX_TYPE_BOX_LAYOUT,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER, _container_interface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_CONTENT_PANE, MplContentPanePrivate))

enum
{
  PROP_0,

  PROP_CHILD,
  PROP_TITLE
};

typedef struct
{
  ClutterActor  *child;
  MxLabel       *title;
} MplContentPanePrivate;

/*
 * ClutterContainer
 */

static ClutterContainerIface *_container_parent_implementation = NULL;

static void
_container_add (ClutterContainer *container,
                ClutterActor     *actor)
{
  MplContentPanePrivate *priv = GET_PRIVATE (container);

  if (priv->child)
  {
    clutter_actor_destroy (priv->child);
    priv->child = NULL;
  }

  if (actor)
  {
    _container_parent_implementation->add (container, actor);
    priv->child = actor;
  }
}

static void
_container_interface_init (ClutterContainerIface *interface)
{
  _container_parent_implementation = g_type_interface_peek_parent (interface);

  interface->add = _container_add;
}

/*
 * MplContentPane
 */

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_CHILD:
    g_value_set_object (value,
                        mpl_content_pane_get_child (MPL_CONTENT_PANE (object)));
    break;
  case PROP_TITLE:
    g_value_set_string (value,
                        mpl_content_pane_get_title (MPL_CONTENT_PANE (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_CHILD:
    mpl_content_pane_set_child (MPL_CONTENT_PANE (object),
                                g_value_get_object (value));
    break;
  case PROP_TITLE:
    mpl_content_pane_set_title (MPL_CONTENT_PANE (object),
                                g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mpl_content_pane_class_init (MplContentPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MplContentPanePrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child",
                                                        "Child",
                                                        "Contained actor",
                                                        CLUTTER_TYPE_ACTOR,
                                                        param_flags));
  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "Title text",
                                                        "",
                                                        param_flags));
}

static void
mpl_content_pane_init (MplContentPane *self)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  priv->title = (MxLabel *) mx_label_new ();
  _container_parent_implementation->add (CLUTTER_CONTAINER (self),
                                         (ClutterActor *) priv->title);
}

ClutterActor *
mpl_content_pane_new (char const *title)
{
  return g_object_new (MPL_TYPE_CONTENT_PANE,
                       "title", title,
                       NULL);
}

char const *
mpl_content_pane_get_title (MplContentPane *self)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPL_IS_CONTENT_PANE (self), NULL);

  return mx_label_get_text (priv->title);
}

void
mpl_content_pane_set_title (MplContentPane  *self,
                            char const      *title)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);
  bool do_notify;

  g_return_if_fail (MPL_IS_CONTENT_PANE (self));

  do_notify = g_strcmp0 (title, mx_label_get_text (priv->title));

  mx_label_set_text (priv->title, title);

  if (do_notify)
    g_object_notify ((GObject *) self, "title");
}

ClutterActor *
mpl_content_pane_get_child (MplContentPane *self)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPL_IS_CONTENT_PANE (self), NULL);

  return priv->child;
}

void
mpl_content_pane_set_child (MplContentPane  *self,
                            ClutterActor    *child)
{
  g_return_if_fail (MPL_IS_CONTENT_PANE (self));

  clutter_container_add_actor (CLUTTER_CONTAINER (self), child);

  g_object_notify ((GObject *) self, "child");
}

