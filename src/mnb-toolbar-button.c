/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-button.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
 *
 * Author: Thomas Wood <thomas@linux.intel.com>
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

#include "mnb-toolbar-button.h"
#include "mnb-toolbar.h"

G_DEFINE_TYPE (MnbToolbarButton, mnb_toolbar_button, MX_TYPE_BUTTON)

#define MNB_TOOLBAR_BUTTON_GET_PRIVATE(obj)                     \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj),                          \
                                MNB_TYPE_TOOLBAR_BUTTON,        \
                                MnbToolbarButtonPrivate))

struct _MnbToolbarButtonPrivate
{
  gchar   *tooltip;
  gboolean in_tooltip_change : 1;
};

static void
mnb_toolbar_button_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             ClutterAllocationFlags flags)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (actor)->priv;
  gfloat nat_width;

  clutter_actor_get_preferred_width (actor, -1, NULL, &nat_width);

  priv->in_tooltip_change = TRUE;

  if ((box->x2 - box->x1) < nat_width)
    mx_widget_set_tooltip_text (MX_WIDGET (actor), priv->tooltip);
  else
    mx_widget_set_tooltip_text (MX_WIDGET (actor), NULL);

  priv->in_tooltip_change = FALSE;

  CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->allocate (actor, box, flags);
}

static void
mnb_toolbar_button_tooltip_text_cb (MnbToolbarButton *button,
                                    GParamSpec *pspec,
                                    gpointer data)
{
  MnbToolbarButtonPrivate *priv = button->priv;
  const gchar *tooltip = mx_widget_get_tooltip_text (MX_WIDGET (button));

  if (tooltip && !priv->in_tooltip_change)
    {
      g_free (priv->tooltip);
      priv->tooltip = g_strdup (tooltip);
    }
}

static void
mnb_toolbar_button_constructed (GObject *self)
{
  MnbToolbarButton *button = MNB_TOOLBAR_BUTTON (self);
  const gchar *tooltip;

  if (G_OBJECT_CLASS (mnb_toolbar_button_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_toolbar_button_parent_class)->constructed (self);

  mx_widget_set_tooltip_delay (MX_WIDGET (self), 0);
  mx_button_set_icon_name (MX_BUTTON (self), "player_play");
  mx_stylable_set_style_class (MX_STYLABLE (button),
                               "ToolbarButton");

  tooltip = mx_widget_get_tooltip_text (MX_WIDGET (button));
  button->priv->tooltip = g_strdup (tooltip);
  g_signal_connect (MX_WIDGET (button), "notify::tooltip-text",
                    G_CALLBACK (mnb_toolbar_button_tooltip_text_cb),
                    NULL);
}

static void
mnb_toolbar_button_finalize (GObject *self)
{
  MnbToolbarButton *button = MNB_TOOLBAR_BUTTON (self);

  g_free (button->priv->tooltip);
  button->priv->tooltip = NULL;

  G_OBJECT_CLASS (mnb_toolbar_button_parent_class)->finalize (self);
}

static void
mnb_toolbar_button_class_init (MnbToolbarButtonClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarButtonPrivate));

  G_OBJECT_CLASS (klass)->constructed = mnb_toolbar_button_constructed;
  G_OBJECT_CLASS (klass)->finalize    = mnb_toolbar_button_finalize;

  actor_class->allocate = mnb_toolbar_button_allocate;
}

static void
mnb_toolbar_button_init (MnbToolbarButton *self)
{
  self->priv = MNB_TOOLBAR_BUTTON_GET_PRIVATE (self);
}

ClutterActor*
mnb_toolbar_button_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR_BUTTON, NULL);
}
