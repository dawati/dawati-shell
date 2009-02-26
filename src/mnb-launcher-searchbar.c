/*
 * Copyright (C) 2009 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@openedhand.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

#include "mnb-launcher-searchbar.h"
#include "moblin-netbook.h"

#define MNB_LAUNCHER_SEARCHBAR_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_LAUNCHER_SEARCHBAR, MnbLauncherSearchbarPrivate))

enum
{
  PROP_0,

  PROP_TEXT
};

enum
{
  ACTIVATED,

  LAST_SIGNAL
};

struct _MnbLauncherSearchbarPrivate
{
  NbtkWidget  *table;
  NbtkWidget  *label;
  NbtkWidget  *entry;
  NbtkWidget  *button;
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncherSearchbar, mnb_launcher_searchbar, NBTK_TYPE_WIDGET)

static void
search_clicked_cb (NbtkButton           *button,
                   MnbLauncherSearchbar *self)
{
  g_signal_emit (self, _signals[ACTIVATED], 0);
}

static void
mnb_launcher_searchbar_set_property (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MnbLauncherSearchbar *self = MNB_LAUNCHER_SEARCHBAR (gobject);

  switch (prop_id)
    {
    case PROP_TEXT:
      nbtk_entry_set_text (NBTK_ENTRY (self->priv->entry),
                           g_value_get_string (value));
      /* TODO do search? */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_launcher_searchbar_get_property (GObject    *gobject,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MnbLauncherSearchbar *self = MNB_LAUNCHER_SEARCHBAR (gobject);

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value,
                          nbtk_entry_get_text (NBTK_ENTRY (self->priv->entry)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_launcher_searchbar_class_init (MnbLauncherSearchbarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
/*  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass); */

  g_type_class_add_private (klass, sizeof (MnbLauncherSearchbarPrivate));

  gobject_class->set_property = mnb_launcher_searchbar_set_property;
  gobject_class->get_property = mnb_launcher_searchbar_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        "Text",
                                                        "Search entry text",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  _signals[ACTIVATED] = g_signal_new ("activated",
                                      G_TYPE_FROM_CLASS (klass),
                                      G_SIGNAL_RUN_LAST,
                                      G_STRUCT_OFFSET (MnbLauncherSearchbarClass, activated),
                                      NULL, NULL,
                                      g_cclosure_marshal_VOID__VOID,
                                      G_TYPE_NONE, 0);
}

static void
mnb_launcher_searchbar_init (MnbLauncherSearchbar *self)
{
  self->priv = MNB_LAUNCHER_SEARCHBAR_GET_PRIVATE (self);

  self->priv->table = nbtk_table_new ();
  clutter_container_add (CLUTTER_CONTAINER (self),
                         CLUTTER_ACTOR (self->priv->table), NULL);

  self->priv->label = nbtk_label_new (_("Applications"));
  nbtk_table_add_widget (NBTK_TABLE (self->priv->table), self->priv->label,
                         0, 0);

  self->priv->entry = nbtk_entry_new ("");
  clutter_actor_set_width (CLUTTER_ACTOR (self->priv->entry), 150);
//  nbtk_table_add_widget (NBTK_TABLE (self->priv->table), self->priv->entry,
//                         0, 1);
  nbtk_table_add_widget_full (NBTK_TABLE (self->priv->table), self->priv->entry,
                              0, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL| NBTK_Y_FILL,
                              0., 0.5);

  self->priv->button = nbtk_button_new_with_label (_("Search"));
  nbtk_table_add_widget (NBTK_TABLE (self->priv->table), self->priv->button,
                         0, 2);
  g_signal_connect (self->priv->button, "clicked",
                    G_CALLBACK (search_clicked_cb), self);

}

NbtkWidget *
mnb_launcher_searchbar_new (void)
{
  return g_object_new (MNB_TYPE_LAUNCHER_SEARCHBAR, NULL);
}
