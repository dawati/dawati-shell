/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Matthew Allum <mallum@linux.intel.com>
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

#include <gtk/gtk.h>
#include <string.h>

#include "mnb-notification.h"
#include "moblin-netbook-notify-store.h"

G_DEFINE_TYPE (MnbNotification, mnb_notification, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
   MNB_TYPE_NOTIFICATION, \
   MnbNotificationPrivate))

enum {
  CLOSED,
  ACTION,
  N_SIGNALS,
};

enum {
  PROP_0,

  PROP_ICON,
  PROP_TEXT
};

static guint signals[N_SIGNALS] = {0};

struct _MnbNotificationPrivate {
  NbtkWidget   *body;
  NbtkWidget   *summary;
  ClutterActor *icon;
  NbtkWidget   *dismiss_button;

  guint         id;

  GHashTable   *hints;
  gint          timeout;

  gboolean      hide_anim_lock;

};

typedef struct {
  MnbNotification *notification;
  gchar *action;
} ActionData;

static void
mnb_notification_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_set_property (GObject *object, guint property_id,
                               const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_dispose (GObject *object)
{
#if 0
  MnbNotificationPrivate *priv = GET_PRIVATE (object);

  /* FIXME : free up others.. */
  if (priv->icon)
    clutter_actor_destroy (priv->icon);

  if (priv->body)
    clutter_actor_destroy (CLUTTER_ACTOR(priv->body));

  if (priv->summary)
    clutter_actor_destroy (CLUTTER_ACTOR(priv->summary));

  if (priv->dismiss_button)
    clutter_actor_destroy (CLUTTER_ACTOR(priv->dismiss_button));

  if (priv->action_button)
    clutter_actor_destroy (CLUTTER_ACTOR(priv->action_button));
#endif

  G_OBJECT_CLASS (mnb_notification_parent_class)->dispose (object);
}

static void
mnb_notification_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_parent_class)->finalize (object);
}

static void
mnb_notification_class_init (MnbNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNotificationPrivate));

  object_class->get_property = mnb_notification_get_property;
  object_class->set_property = mnb_notification_set_property;
  object_class->dispose = mnb_notification_dispose;
  object_class->finalize = mnb_notification_finalize;

  signals[CLOSED]
    = g_signal_new ("closed",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (MnbNotificationClass, closed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

  signals[ACTION]
    = g_signal_new ("action",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (MnbNotificationClass, action),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING);

}

static void
on_dismiss_click (ClutterActor *button, MnbNotification *self)
{
  g_signal_emit (self, signals[CLOSED], 0);
}

static void
on_action_click (ClutterActor *button, ActionData *data)
{
  g_signal_emit (data->notification, signals[ACTION], 0, data->action);
}


static void
mnb_notification_init (MnbNotification *self)
{
  MnbNotificationPrivate *priv;
  ClutterText *txt;

  priv = self->priv = GET_PRIVATE (self);

  nbtk_widget_set_style_class_name (NBTK_WIDGET (self), "Notification");
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 4);
  nbtk_table_set_row_spacing (NBTK_TABLE (self), 4);

  priv->dismiss_button = nbtk_button_new ();
  priv->icon           = clutter_texture_new ();
  priv->body           = nbtk_label_new ("");
  priv->summary        = nbtk_label_new ("");

  nbtk_table_add_actor (NBTK_TABLE (self), priv->icon, 0, 0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->icon),
                               "keep-aspect-ratio", TRUE,
                               "y-expand", FALSE,
                               "x-expand", FALSE,
                               "x-align", 0.0,
                               NULL);

  txt = CLUTTER_TEXT(nbtk_label_get_clutter_text(NBTK_LABEL(priv->summary)));
  clutter_text_set_line_alignment (CLUTTER_TEXT (txt), PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt), PANGO_ELLIPSIZE_END);

  nbtk_table_add_widget (NBTK_TABLE (self), priv->summary, 0, 1);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->summary),
                               "y-expand", TRUE,
                               "x-expand", TRUE,
                               NULL);

  nbtk_table_add_widget (NBTK_TABLE (self), priv->body, 1, 0);
  nbtk_table_set_widget_colspan (NBTK_TABLE (self), priv->body, 2);

  txt = CLUTTER_TEXT(nbtk_label_get_clutter_text(NBTK_LABEL(priv->body)));
  clutter_text_set_line_alignment (CLUTTER_TEXT (txt), PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt), PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->body),
                               "y-expand", FALSE,
                               "x-expand", FALSE,
                               NULL);

  nbtk_button_set_label (NBTK_BUTTON (priv->dismiss_button), "Dismiss");
  nbtk_table_add_widget (NBTK_TABLE (self), priv->dismiss_button, 2, 1);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->dismiss_button),
                               "y-expand", FALSE,
                               "x-expand", FALSE,
                               "x-align",  0.0,
                               NULL);

  g_signal_connect (priv->dismiss_button, "clicked",
                    G_CALLBACK (on_dismiss_click), self);

  nbtk_widget_set_style_class_name (priv->summary,
                                    "NotificationSummary");
  nbtk_widget_set_style_class_name (priv->body,
                                    "NotificationBody");

}

NbtkWidget*
mnb_notification_new (void)
{
  return g_object_new (MNB_TYPE_NOTIFICATION,
                       "show-on-set-parent", FALSE,
                       "reactive", TRUE,
                       NULL);
}

void
mnb_notification_update (MnbNotification *notification,
                         Notification    *details)
{
  MnbNotificationPrivate *priv;
  gboolean                has_action = FALSE;

  g_return_if_fail (MNB_IS_NOTIFICATION (notification));

  priv = GET_PRIVATE (notification);

  priv->id = details->id;

  if (details->summary)
    nbtk_label_set_text (NBTK_LABEL(priv->summary), details->summary);

  if (details->body)
    nbtk_label_set_text (NBTK_LABEL(priv->body), details->body);

  if (details->icon_name)
    {
      GtkIconTheme           *theme;
      GtkIconInfo            *info;

      theme = gtk_icon_theme_get_default ();
      info = gtk_icon_theme_lookup_icon (theme, details->icon_name, 48, 0);

      if (info)
        {
          clutter_texture_set_from_file (CLUTTER_TEXTURE(priv->icon),
                                         gtk_icon_info_get_filename (info),
                                         NULL);
          gtk_icon_info_free (info);
        }
    }

  if (details->actions)
    {
      ClutterActor *layout = NULL;
      GHashTableIter iter;
      gchar *key, *value;

      g_hash_table_iter_init (&iter, details->actions);
      while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&value))
        {
          if (strcasecmp(key, "default"))
            {
              ActionData *data;
              NbtkWidget *button;

              if (layout == NULL)
                {
                  layout = g_object_new (NBTK_TYPE_GRID,
                                         /*
                                           "x", 5,
                                           "y", 5,
                                           "width", 0,
                                         */
                                         NULL);

                  nbtk_table_add_widget (NBTK_TABLE (notification),
                                         NBTK_WIDGET (layout), 2, 0);
                }

              data = g_slice_new (ActionData);
              data->notification = notification;
              data->action = g_strdup (key);

              button = nbtk_button_new ();

              nbtk_button_set_label (NBTK_BUTTON (button), value);

              clutter_container_add_actor (CLUTTER_CONTAINER (layout),
                                           CLUTTER_ACTOR(button));

              g_signal_connect (button, "clicked",
                                G_CALLBACK (on_action_click), data);

              has_action = TRUE;
            }
        }
    }

  if (details->is_urgent)
    {
      /* Essentially change title color to Red and  remove dismiss button */
      nbtk_widget_set_style_class_name (priv->summary,
                                        "NotificationSummaryUrgent");

      if (has_action == TRUE)
        {
          /* Remove the dismiss button.. */
          clutter_container_remove_actor(CLUTTER_CONTAINER (notification),
                                         CLUTTER_ACTOR (priv->dismiss_button));
        }
    }
}

guint
mnb_notification_get_id (MnbNotification *notification)
{
  MnbNotificationPrivate *priv;

  g_return_val_if_fail (MNB_IS_NOTIFICATION (notification), 0);

  priv = GET_PRIVATE (notification);

  return priv->id;
}

