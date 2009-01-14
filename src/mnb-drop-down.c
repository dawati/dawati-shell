/*
 * Copyright (c) 2008 Intel Corp.
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

#include "mnb-drop-down.h"
#define MNB_PADDING(a, b, c, d) {CLUTTER_UNITS_FROM_INT (a), CLUTTER_UNITS_FROM_INT (b), \
                                 CLUTTER_UNITS_FROM_INT (c), CLUTTER_UNITS_FROM_INT (d) }

G_DEFINE_TYPE (MnbDropDown, mnb_drop_down, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_DROP_DOWN, MnbDropDownPrivate))


struct _MnbDropDownPrivate {
    ClutterActor *child;
};

static void
mnb_drop_down_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_drop_down_parent_class)->dispose (object);
}

static void
mnb_drop_down_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_drop_down_parent_class)->finalize (object);
}

static void
mnb_drop_down_class_init (MnbDropDownClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbDropDownPrivate));

  object_class->get_property = mnb_drop_down_get_property;
  object_class->set_property = mnb_drop_down_set_property;
  object_class->dispose = mnb_drop_down_dispose;
  object_class->finalize = mnb_drop_down_finalize;
}

static void
mnb_drop_down_init (MnbDropDown *self)
{
  NbtkWidget *footer, *up_button;
  NbtkPadding padding = MNB_PADDING (4, 4, 4, 4);

  self->priv = GET_PRIVATE (self);

  /* footer with "up" button */
  footer = nbtk_table_new ();
  nbtk_widget_set_padding (footer, &padding);
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");

  up_button = nbtk_button_new ();
  nbtk_widget_set_style_class_name (up_button, "drop-down-up-button");
  nbtk_table_add_actor (NBTK_TABLE (footer), CLUTTER_ACTOR (up_button), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (up_button), 23, 21);
  clutter_container_child_set (CLUTTER_CONTAINER (footer),
                               CLUTTER_ACTOR (up_button),
                               "keep-aspect-ratio", TRUE,
                               "x-align", 1.0,
                               NULL);
  g_signal_connect_swapped (up_button, "clicked",
                            G_CALLBACK (clutter_actor_hide), self);

  nbtk_table_add_widget (NBTK_TABLE (self), footer, 1, 0);
}

MnbDropDown*
mnb_drop_down_new (void)
{
  return g_object_new (MNB_TYPE_DROP_DOWN, NULL);
}

void
mnb_drop_down_set_child (MnbDropDown *drop_down,
                         ClutterActor *child)
{
  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  if (drop_down->priv->child)
    {
      clutter_container_remove (CLUTTER_CONTAINER (drop_down), child);
    }

  nbtk_table_add_actor (NBTK_TABLE (drop_down), child, 0, 0);
  drop_down->priv->child = child;
}

ClutterActor*
mnb_drop_down_get_child (MnbDropDown *drop_down)
{
  g_return_if_fail (MNB_DROP_DOWN (drop_down));

  return drop_down->priv->child;
}
