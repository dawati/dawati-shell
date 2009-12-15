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
#include <glib/gi18n.h>

#include "mnb-notification.h"
#include "moblin-netbook-notify-store.h"

G_DEFINE_TYPE (MnbNotification, mnb_notification, MX_TYPE_TABLE)

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
  ClutterActor   *body;
  ClutterActor   *summary;
  ClutterActor *icon;
  ClutterActor *dismiss_button;
  ClutterActor *button_box;
  ClutterActor *title_box;

  guint         id;


  GHashTable   *hints;
  gint          timeout;
  guint         timeout_id;

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
  MnbNotificationPrivate *priv = GET_PRIVATE (object);

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  if (priv->icon)
    {
      g_object_unref (priv->icon);
      priv->icon = NULL;
    }

  G_OBJECT_CLASS (mnb_notification_parent_class)->dispose (object);
}

static void
mnb_notification_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_parent_class)->finalize (object);
}

static gboolean
notification_timeout (MnbNotification *notification)
{
  g_signal_emit (notification, signals[CLOSED], 0);
  return FALSE;
}

static void
mnb_notification_show (ClutterActor *actor)
{
  MnbNotificationPrivate *priv;

  priv = GET_PRIVATE (actor);

  if (priv->timeout > 0)
    {
      priv->timeout_id = g_timeout_add (priv->timeout,
                                        (GSourceFunc)notification_timeout,
                                        MNB_NOTIFICATION(actor));
    }

  CLUTTER_ACTOR_CLASS (mnb_notification_parent_class)->show (actor);
}

static void
mnb_notification_class_init (MnbNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class    = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNotificationPrivate));

  object_class->get_property = mnb_notification_get_property;
  object_class->set_property = mnb_notification_set_property;
  object_class->dispose = mnb_notification_dispose;
  object_class->finalize = mnb_notification_finalize;

  actor_class->show = mnb_notification_show;

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

  mx_widget_set_style_class_name (MX_WIDGET (self), "Notification");
  mx_table_set_col_spacing (MX_TABLE (self), 4);
  mx_table_set_row_spacing (MX_TABLE (self), 8);

  priv->dismiss_button = mx_button_new ();
  priv->icon           = g_object_ref_sink (clutter_texture_new ());
  priv->body           = mx_label_new ("");
  priv->summary        = mx_label_new ("");
  priv->title_box      = mx_table_new ();


  mx_table_set_col_spacing (MX_TABLE (priv->title_box), 4);
  mx_table_add_actor (MX_TABLE (self), CLUTTER_ACTOR (priv->title_box), 0, 0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->title_box),
                               "y-expand", FALSE,
                               "x-expand", TRUE,
                               NULL);

  mx_table_add_actor (MX_TABLE (priv->title_box), priv->icon, 0, 0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                               CLUTTER_ACTOR (priv->icon),
                               "y-expand", FALSE,
                               "x-expand", FALSE,
                               "x-align", 0.0,
                               "x-fill", FALSE,
                               "y-fill", FALSE,
                               NULL);

  txt = CLUTTER_TEXT(mx_label_get_clutter_text(MX_LABEL(priv->summary)));
  clutter_text_set_line_alignment (CLUTTER_TEXT (txt), PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt), PANGO_ELLIPSIZE_END);

  mx_table_add_actor (MX_TABLE (priv->title_box), CLUTTER_ACTOR (priv->summary), 0, 1);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                               CLUTTER_ACTOR (priv->summary),
                               "y-expand", TRUE,
                               "x-expand", TRUE,
                               "y-align", 0.5,
                               "x-align", 0.0,
                               "y-fill", FALSE,
                               "x-fill", FALSE,
                               NULL);

  mx_table_add_actor (MX_TABLE (self), CLUTTER_ACTOR (priv->body), 1, 0);

  txt = CLUTTER_TEXT(mx_label_get_clutter_text(MX_LABEL(priv->body)));
  clutter_text_set_line_alignment (CLUTTER_TEXT (txt), PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt), PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);
  clutter_text_set_use_markup (CLUTTER_TEXT (txt), TRUE);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->body),
                               "y-expand", FALSE,
                               "x-expand", FALSE,
                               NULL);

  mx_button_set_label (MX_BUTTON (priv->dismiss_button), _("Dismiss"));

  /* create the box for the buttons */
  priv->button_box = mx_grid_new ();
  mx_grid_set_end_align (MX_GRID (priv->button_box), TRUE);
  mx_grid_set_column_spacing (MX_GRID (priv->button_box), 7);
  mx_table_add_actor (MX_TABLE (self), CLUTTER_ACTOR (priv->button_box),
                        2, 0);

  /* add the dismiss button to the button box */
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               CLUTTER_ACTOR (priv->dismiss_button));

  g_signal_connect (priv->dismiss_button, "clicked",
                    G_CALLBACK (on_dismiss_click), self);

  mx_widget_set_style_class_name (MX_WIDGET (priv->summary),
                                    "NotificationSummary");
  mx_widget_set_style_class_name (MX_WIDGET (priv->body),
                                    "NotificationBody");

}

ClutterActor*
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
  gboolean                has_action = FALSE, no_icon = TRUE;
  GList                  *l;

  g_return_if_fail (MNB_IS_NOTIFICATION (notification));

  priv = GET_PRIVATE (notification);

  priv->id = details->id;
  priv->timeout = details->timeout_ms;

  if (details->summary)
    mx_label_set_text (MX_LABEL(priv->summary), details->summary);

  if (details->body)
    mx_label_set_text (MX_LABEL(priv->body), details->body);

  if (details->icon_pixbuf)
    {
      GdkPixbuf *pixbuf = details->icon_pixbuf;

      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (priv->icon),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ?
                                         4 : 3,
                                         0, NULL);
      no_icon = FALSE;
    }
  else if (details->icon_name)
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
          no_icon = FALSE;
        }
    }

  /* Since notifications can be "updated" we need to be able reverse any operations */
  if (no_icon && clutter_actor_get_parent (priv->icon))
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->title_box),
                                      priv->icon);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                                   CLUTTER_ACTOR (priv->summary),
                                   "col", 0,
                                   NULL);
    }
  else if (!clutter_actor_get_parent (priv->icon))
    {
      mx_table_add_actor (MX_TABLE (priv->title_box), priv->icon, 0, 0);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                                   CLUTTER_ACTOR (priv->icon),
                                   "y-expand", FALSE,
                                   "x-expand", FALSE,
                                   "x-align", 0.0,
                                   "x-fill", FALSE,
                                   "y-fill", FALSE,
                                   NULL);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                                   CLUTTER_ACTOR (priv->summary),
                                   "col", 1,
                                   NULL);
    }

  if (details->actions)
    {
      GHashTableIter iter;
      gchar *key, *value;

      /* Remove all except default / Dismiss action. We must do this to
       * support "updating".
       */
      for (l = clutter_container_get_children (CLUTTER_CONTAINER (priv->button_box));
           l;
           l = g_list_delete_link (l, l))
        {
          if (l->data != priv->dismiss_button)
            clutter_container_remove_actor (CLUTTER_CONTAINER (priv->button_box),
                                            CLUTTER_ACTOR (l->data));
        }

      g_hash_table_iter_init (&iter, details->actions);
      while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&value))
        {
          if (strcasecmp(key, "default") != 0)
            {
              ActionData *data;
              ClutterActor *button;

              data = g_slice_new (ActionData);
              data->notification = notification;
              data->action = g_strdup (key);

              button = mx_button_new ();

              mx_button_set_label (MX_BUTTON (button), value);

              clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                                           CLUTTER_ACTOR (button));

              g_signal_connect (button, "clicked",
                                G_CALLBACK (on_action_click), data);

              has_action = TRUE;
            }
        }
    }

  if (details->is_urgent)
    {
      mx_widget_set_style_class_name (MX_WIDGET (priv->summary),
                                        "NotificationSummaryUrgent");
    }
  else
    {
      mx_widget_set_style_class_name (MX_WIDGET (priv->summary),
                                        "NotificationSummary");
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

