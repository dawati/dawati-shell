/*
 * Copyright (C) 2008 Intel Corporation
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
 * Written by: Robert Staudinger <robsta@o-hand.com>.
 * Based on nbtk-label.c by Thomas Wood <thomas@linux.intel.com>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mnb-launcher-button.h"

#define MNB_LAUNCHER_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonPrivate))

enum
{
  ACTIVATED,

  LAST_SIGNAL
};

struct _MnbLauncherButtonPrivate
{
  NbtkWidget    *table;
  ClutterActor  *icon;
  NbtkLabel     *app_name;
  NbtkLabel     *category;
  NbtkLabel     *comment;

  guint is_pressed : 1;
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncherButton, mnb_launcher_button, NBTK_TYPE_WIDGET)

static void
dispose (GObject *object)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (object);

  if (self->priv->table)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->table));
      self->priv->table = NULL;
    }

  if (self->priv->icon)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->icon));
      self->priv->icon = NULL;
    }

  if (self->priv->app_name)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->app_name));
      self->priv->app_name = NULL;
    }

  if (self->priv->category)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->category));
      self->priv->category = NULL;
    }

  if (self->priv->comment)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->comment));
      self->priv->comment = NULL;
    }

  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->dispose (object);
}

static gboolean
button_press_event (ClutterActor       *actor,
                    ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      self->priv->is_pressed = TRUE;
      clutter_grab_pointer (actor);
      return TRUE;
    }

  return FALSE;
}

static gboolean
button_release_event (ClutterActor       *actor,
                      ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      if (!self->priv->is_pressed)
        return FALSE;

      clutter_ungrab_pointer ();
      self->priv->is_pressed = FALSE;
      g_signal_emit (self, _signals[ACTIVATED], 0, event);

      return TRUE;
    }

  return FALSE;
}

static gboolean
leave_event (ClutterActor         *actor,
             ClutterCrossingEvent *event)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  if (self->priv->is_pressed)
    {
      clutter_ungrab_pointer ();
      self->priv->is_pressed = FALSE;
    }

  return FALSE;
}

static void
mnb_launcher_button_class_init (MnbLauncherButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherButtonPrivate));

  object_class->dispose = dispose;

  actor_class->button_press_event = button_press_event;
  actor_class->button_release_event = button_release_event;
  actor_class->leave_event = leave_event;

  _signals[ACTIVATED] = g_signal_new ("activated",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (MnbLauncherButtonClass, activated),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__POINTER,
                                    G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mnb_launcher_button_init (MnbLauncherButton *self)
{
  self->priv = MNB_LAUNCHER_BUTTON_GET_PRIVATE (self);

  self->priv->table = nbtk_table_new ();
  clutter_container_add (CLUTTER_CONTAINER (self),
                         CLUTTER_ACTOR (self->priv->table),
                         NULL);

  self->priv->icon = NULL;

  self->priv->app_name = (NbtkLabel *) nbtk_label_new (NULL);
  nbtk_table_add_widget (NBTK_TABLE (self->priv->table),
                         NBTK_WIDGET (self->priv->app_name),
                         0, 1);

  self->priv->category = (NbtkLabel *) nbtk_label_new (NULL);
  nbtk_table_add_widget (NBTK_TABLE (self->priv->table),
                         NBTK_WIDGET (self->priv->category),
                         1, 1);

  self->priv->comment = (NbtkLabel *) nbtk_label_new (NULL);
  nbtk_table_add_widget (NBTK_TABLE (self->priv->table),
                         NBTK_WIDGET (self->priv->comment),
                         2, 1);
}

NbtkWidget *
mnb_launcher_button_new (const gchar *icon_file,
                         gint         icon_size,
                         const gchar *app_name,
                         const gchar *category,
                         const gchar *comment)
{
  MnbLauncherButton *self;
  GError            *error;

  self = g_object_new (MNB_TYPE_LAUNCHER_BUTTON, NULL);

  error = NULL;
  self->priv->icon = clutter_texture_new_from_file (icon_file, &error);
  if (error) {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (self->priv->icon) {

    if (icon_size > -1) {
      clutter_actor_set_size (self->priv->icon, icon_size, icon_size);
      g_object_set (G_OBJECT (self->priv->icon), "sync-size", TRUE, NULL);
    }

    nbtk_table_add_actor_full (NBTK_TABLE (self->priv->table),
                               self->priv->icon,
                               0, 0, 3, 1,
                               NBTK_KEEP_ASPECT_RATIO | NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                               0.5, 0.5);
  }

  if (app_name)
    nbtk_label_set_text (self->priv->app_name, app_name);

  if (category)
    nbtk_label_set_text (self->priv->category, category);

  if (comment)
    nbtk_label_set_text (self->priv->comment, comment);

  return NBTK_WIDGET (self);
}
