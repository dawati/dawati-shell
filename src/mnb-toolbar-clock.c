/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-icon.c */
/*
 * Copyright (c) 2010 Intel Corp.
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

#include <glib/gi18n.h>

#include "mnb-toolbar-clock.h"
#include "mnb-toolbar.h"

#define MNB_24H_KEY_DIR "/apps/date-time-panel"
#define MNB_24H_KEY MNB_24H_KEY_DIR "/24_h_clock"

G_DEFINE_TYPE (MnbToolbarClock, mnb_toolbar_clock, MX_TYPE_BUTTON)


#define MNB_TOOLBAR_CLOCK_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_TOOLBAR_CLOCK, MnbToolbarClockPrivate))

static void mnb_toolbar_clock_update_time_date (MnbToolbarClock *clock);

struct _MnbToolbarClockPrivate
{
  guint         timeout_id;
  gulong        toolbar_show_id;

  guint disposed    : 1;
  guint initialized : 1;
};

static void
mnb_toolbar_clock_dispose (GObject *object)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  if (priv->toolbar_show_id)
    {
      MetaPlugin *plugin = dawati_netbook_get_plugin_singleton ();
      DawatiNetbookPluginPrivate *ppriv = DAWATI_NETBOOK_PLUGIN (plugin)->priv;

      g_signal_handler_disconnect (ppriv->toolbar, priv->toolbar_show_id);
      priv->toolbar_show_id = 0;
    }

  G_OBJECT_CLASS (mnb_toolbar_clock_parent_class)->dispose (object);
}

static void
mnb_toolbar_clock_format_changed_cb (GConfClient *client,
                                     guint        cnxn_id,
                                     GConfEntry  *entry,
                                     gpointer     data)
{
  MnbToolbarClock *clock = MNB_TOOLBAR_CLOCK (data);

  mnb_toolbar_clock_update_time_date (clock);
}

static void
mnb_toolbar_clock_update_time_date (MnbToolbarClock *clock)
{
  MnbToolbarClockPrivate *priv = clock->priv;

  time_t           t;
  struct tm       *tmp;
  char             time_str[64];
  static gboolean  setup_done = FALSE;
  GConfClient     *client;
  char            *time_ptr;

  if (priv->disposed)
    return;

  client = gconf_client_get_default ();

  t = time (NULL);
  tmp = localtime (&t);
  if (tmp)
    {
      gboolean c24h = gconf_client_get_bool (client, MNB_24H_KEY, NULL);

      if (c24h)
        {

          /* translators: translate this to a suitable 24 hourt time format for
           * your locale showing only hours and minutes. For available format
           * specifiers see
           * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
           */
          strftime (time_str, 64, _("%H:%M"), tmp);
        }
      else
        {
          /* translators: translate this to a suitable default time format for
           * your locale showing only hours and minutes. For available format
           * specifiers see
           * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
           */
          strftime (time_str, 64, _("%l:%M %P"), tmp);
        }
    }
  else
    snprintf (time_str, 64, "Time");

  /*
   * Strip leading space, if any.
   */
  if (time_str[0] == ' ')
    time_ptr = &time_str[1];
  else
    time_ptr = &time_str[0];

  mx_button_set_label (MX_BUTTON (clock), time_ptr);

  if (tmp)
    /* translators: translate this to a suitable date format for your locale.
     * For availabe format specifiers see
     * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
     */
    strftime (time_str, 64, _("%B %e, %Y"), tmp);
  else
    snprintf (time_str, 64, "Date");

  mx_widget_set_tooltip_text (MX_WIDGET (clock), time_str);

  if (!setup_done)
    {
      GError *error = NULL;

      setup_done = TRUE;

      gconf_client_add_dir (client, MNB_24H_KEY_DIR,
                            GCONF_CLIENT_PRELOAD_NONE,
                            &error);

      if (error)
        {
          g_warning (G_STRLOC ": Error when adding directory "
                     MNB_24H_KEY_DIR " for notification: %s",
                     error->message);
          g_clear_error (&error);
        }

      gconf_client_notify_add (client,
                               MNB_24H_KEY,
                               mnb_toolbar_clock_format_changed_cb,
                               clock,
                               NULL,
                               &error);

      if (error)
        {
          g_warning (G_STRLOC ": Error when adding key "
                     MNB_24H_KEY " for notification: %s",
                     error->message);
          g_clear_error (&error);
        }
    }

  g_object_unref (client);
}

static gboolean
mnb_toolbar_clock_timeout_cb (MnbToolbarClock *clock)
{
  MnbToolbarClockPrivate *priv = clock->priv;

  mnb_toolbar_clock_update_time_date (clock);

  if (!priv->initialized)
    {
      priv->initialized = TRUE;

      priv->timeout_id =
        g_timeout_add_seconds (60, (GSourceFunc) mnb_toolbar_clock_timeout_cb,
                               clock);

      return FALSE;
    }

  return TRUE;
}

static void
mnb_toolbar_clock_constructed (GObject *self)
{
  MnbToolbarClockPrivate     *priv = MNB_TOOLBAR_CLOCK (self)->priv;
  ClutterActor               *actor = CLUTTER_ACTOR (self);
  time_t                      interval = 60 - (time(NULL) % 60);

  if (G_OBJECT_CLASS (mnb_toolbar_clock_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_toolbar_clock_parent_class)->constructed (self);

  clutter_actor_push_internal (actor);

  clutter_actor_pop_internal (actor);

  mnb_toolbar_clock_update_time_date (MNB_TOOLBAR_CLOCK (self));

  if (!interval)
    {
      priv->initialized = TRUE;
      priv->timeout_id =
        g_timeout_add_seconds (60,
                               (GSourceFunc) mnb_toolbar_clock_timeout_cb,
                               self);
    }
  else
    {
      priv->timeout_id =
        g_timeout_add_seconds (interval,
                               (GSourceFunc) mnb_toolbar_clock_timeout_cb,
                               self);
    }
}

static void
mnb_toolbar_clock_class_init (MnbToolbarClockClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarClockPrivate));

  object_class->dispose             = mnb_toolbar_clock_dispose;
  object_class->constructed         = mnb_toolbar_clock_constructed;
}

static void
mnb_toolbar_clock_init (MnbToolbarClock *self)
{
  self->priv = MNB_TOOLBAR_CLOCK_GET_PRIVATE (self);
}

ClutterActor*
mnb_toolbar_clock_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR_CLOCK, NULL);
}

