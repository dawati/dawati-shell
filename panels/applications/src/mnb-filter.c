
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

#include <glib/gi18n.h>

#include "mnb-filter.h"

G_DEFINE_TYPE (MnbFilter, mnb_filter, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FILTER, MnbFilterPrivate))

enum
{
  PROP_0,

  PROP_TEXT
};

typedef struct
{
  MxEntry *entry;
} MnbFilterPrivate;

static void
_entry_text_notify_cb (MxEntry    *entry,
                       GParamSpec *pspec,
                       MnbFilter  *self)
{
  g_object_notify (G_OBJECT (self), "text");
}

static void
_entry_clear_clicked_cb (MxEntry    *entry,
                         MnbFilter  *self)
{
  mx_entry_set_text (entry, "");
}

static void
_key_focus_in (ClutterActor *actor)
{
  MnbFilterPrivate *priv = GET_PRIVATE (actor);

  clutter_actor_grab_key_focus ((ClutterActor *) priv->entry);
}

static void
_set_property (GObject      *object,
               guint         prop_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  MnbFilterPrivate *priv = GET_PRIVATE (object);

  switch (prop_id)
  {
  case PROP_TEXT:
    mx_entry_set_text (priv->entry, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
_get_property (GObject    *object,
               guint       prop_id,
               GValue     *value,
               GParamSpec *pspec)
{
  MnbFilterPrivate *priv = GET_PRIVATE (object);

  switch (prop_id)
  {
  case PROP_TEXT:
    g_value_set_string (value, mx_entry_get_text (priv->entry));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_filter_parent_class)->dispose (object);
}

static void
mnb_filter_class_init (MnbFilterClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamFlags        param_flags;

  g_type_class_add_private (klass, sizeof (MnbFilterPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  actor_class->key_focus_in = _key_focus_in;

  /* Properties */

  param_flags = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        "Text",
                                                        "Text in the entry",
                                                        NULL,
                                                        param_flags));
}

static void
mnb_filter_init (MnbFilter *self)
{
  MnbFilterPrivate *priv = GET_PRIVATE (self);
  ClutterActor *button;

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 12);

  priv->entry = (MxEntry *) mx_entry_new ();
  mx_entry_set_secondary_icon_from_file (priv->entry,
                                         THEMEDIR"/clear-entry.png");
  g_signal_connect (priv->entry, "notify::text",
                    G_CALLBACK (_entry_text_notify_cb), self);
  g_signal_connect (priv->entry, "secondary-icon-clicked",
                    G_CALLBACK (_entry_clear_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), (ClutterActor *) priv->entry);
  clutter_container_child_set (CLUTTER_CONTAINER (self), (ClutterActor *) priv->entry,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);

  button = mx_button_new_with_label (_("Search"));
  clutter_container_add_actor (CLUTTER_CONTAINER (self), button);
}

ClutterActor *
mnb_filter_new (void)
{
  return g_object_new (MPD_TYPE_FILTER, NULL);
}

char const *
mnb_filter_get_text (MnbFilter *self)
{
  MnbFilterPrivate *priv = GET_PRIVATE (self);

  return mx_entry_get_text (priv->entry);
}

void
mnb_filter_set_text (MnbFilter  *self,
                     char const *text)
{
  MnbFilterPrivate *priv = GET_PRIVATE (self);

  mx_entry_set_text (priv->entry, text);
}

