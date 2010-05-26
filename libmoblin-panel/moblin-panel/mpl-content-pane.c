
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

/**
 * SECTION:mpl-content-pane
 * @short_description: Content pane with a header.
 * @Title: MplContentPane
 *
 * #MplContentPane provides a styled content pane widget, which consists of a
 *  header with a title, an optional header actor packed at the right edge of
 *  the header, and a content area.
 */

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
  PROP_HEADER_ACTOR,
  PROP_TITLE
};

typedef struct
{
  MxBoxLayout   *header;
  MxLabel       *title;
  ClutterActor  *header_actor;
  ClutterActor  *child;
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
  case PROP_HEADER_ACTOR:
    g_value_set_object (value,
                        mpl_content_pane_get_header_actor (
                          MPL_CONTENT_PANE (object)));
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
  case PROP_HEADER_ACTOR:
    mpl_content_pane_set_header_actor (MPL_CONTENT_PANE (object),
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
                                   PROP_HEADER_ACTOR,
                                   g_param_spec_object ("header-actor",
                                                        "Header actor",
                                                        "Right-hand header actor",
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
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 6);

  priv->header = (MxBoxLayout *) mx_box_layout_new ();
  mx_box_layout_set_spacing (priv->header, 12);
  mx_stylable_set_style_class (MX_STYLABLE (priv->header), "header");
  clutter_actor_set_height ((ClutterActor *) priv->header, 36);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (self),
                           CLUTTER_ACTOR (priv->header), 0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *) priv->header,
                               "expand", FALSE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);

  priv->title = (MxLabel *) mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->title), "title");
  mx_box_layout_add_actor (priv->header, (ClutterActor *) priv->title, 0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->header),
                               (ClutterActor *) priv->title,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", FALSE,
                               NULL);
}

/**
 * mpl_content_pane_new:
 * @title: the panel title
 *
 * Constructs a new #MplContentPane with the given title.
 *
 * Return value: #ClutterActor
 */
ClutterActor *
mpl_content_pane_new (char const *title)
{
  return g_object_new (MPL_TYPE_CONTENT_PANE,
                       "title", title,
                       NULL);
}

/**
 * mpl_content_pane_get_title:
 * @self: #MplContentPane
 *
 * Returns the current title of the pane.
 *
 * Return value: pane title
 */
char const *
mpl_content_pane_get_title (MplContentPane *self)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPL_IS_CONTENT_PANE (self), NULL);

  return mx_label_get_text (priv->title);
}

/**
 * mpl_content_pane_set_title:
 * @self: #MplContentPane
 * @title: title
 *
 * Sets the title of the pane to the given string.
 */
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

/**
 * mpl_content_pane_get_child:
 * @self: #MplContentPane
 *
 * Returns the content child of the pane.
 *
 * Return value: #ClutterActor
 */
ClutterActor *
mpl_content_pane_get_child (MplContentPane *self)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPL_IS_CONTENT_PANE (self), NULL);

  return priv->child;
}

/**
 * mpl_content_pane_set_child:
 * @self: #MplContentPane
 * @child: #ClutterActor
 *
 * Sets the content child of the pane to the provided actor.
 */
void
mpl_content_pane_set_child (MplContentPane  *self,
                            ClutterActor    *child)
{
  g_return_if_fail (MPL_IS_CONTENT_PANE (self));

  clutter_container_add_actor (CLUTTER_CONTAINER (self), child);

  g_object_notify ((GObject *) self, "child");
}

/**
 * mpl_content_pane_get_header_actor:
 * @self: #MplContentPane
 *
 * Returns the pane header actor, if set or %NULL.
 *
 * Return value: #ClutterActor
 */
ClutterActor *
mpl_content_pane_get_header_actor (MplContentPane  *self)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPL_IS_CONTENT_PANE (self), NULL);

  return priv->header_actor;
}

/**
 * mpl_content_pane_set_header_actor:
 * @self: #MplContentPane
 * @actor: #ClutterActor or %NULL
 *
 * Sets the pane header actor. The header actor is an optional actor inserted
 * into the header and packed from its right edge. It can be used, for example,
 * to add a close button to the pane.
 */
void
mpl_content_pane_set_header_actor (MplContentPane *self,
                                   ClutterActor   *actor)
{
  MplContentPanePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPL_IS_CONTENT_PANE (self));

  if (actor != priv->header_actor)
  {
    if (priv->header_actor)
    {
      clutter_actor_destroy (priv->header_actor);
      priv->header_actor = NULL;
    }

    if (actor)
    {
      clutter_container_add_actor (CLUTTER_CONTAINER (priv->header), actor);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->header), actor,
                                   "expand", FALSE,
                                   "x-align", MX_ALIGN_END,
                                   "x-fill", TRUE,
                                   NULL);
      priv->header_actor = actor;
    }

    g_object_notify (G_OBJECT (self), "header-actor");
  }
}

