/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2008, 2009, Intel Corporation.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
 *
 */

#ifndef MOBLIN_NETBOOK_SYSTEM_TRAY_H
#define MOBLIN_NETBOOK_SYSTEM_TRAY_H

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#define MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW  "MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW"

/*
 * Type of the icon; the cline needs to set this to one of "wifi", "volume",
 * "bluetooth", "battery".
 */
#define MOBLIN_SYSTEM_TRAY_TYPE           "MOBLIN_SYSTEM_TRAY_TYPE"

#ifndef MOBLIN_SYSTEM_TRAY_FROM_PLUGIN
/*
 * Utility code to set up Tray code in the application. Simply call
 *
 * mnbk_system_tray_init (icon, config_plug, type);
 *
 * icon: GtkStatusIcon* instance.
 *
 * config_plug: GtkPlug* instance; this is the top level container that
 * holding the application configuration window. Unlike in the normal X
 * tray set up, this window is shown *by the netbook plugin*, not the
 * application, in response to click on the status icon (your application will
 * no receive this click directly).
 *
 * type: string identifying the application; valid options are "wifi",
 * "bluetooth", "sound", "battery".
 *
 * See tests/test-tray.c for an example.
 */

#include <string.h>

struct mnbk_tray_setup_data
{
  GtkStatusIcon *icon;
  GtkPlug       *config;
  gchar         *type;
  guint          init_cb_id;
};

static struct mnbk_tray_setup_data _mnbk_tray_setup_data;

static gboolean
_mnbk_setup_config_window (GtkStatusIcon               *icon,
                           Window                       config_win,
                           struct mnbk_tray_setup_data *tray_data)
{
  Atom tray_atom = gdk_x11_get_xatom_by_name (MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW);
  Atom type_atom = gdk_x11_get_xatom_by_name (MOBLIN_SYSTEM_TRAY_TYPE);
  Window    icon_win;
  gpointer *plug_location;
  GtkPlug  *plug;

  /*
   * This is very, very evil, but GtkStatusIcon does not provide API
   * to get at the icon xwindow ...
   *
   * The tray icon is a GtkPlug subclass, and is stored as the first
   * item in the private structure of GtkSystemIcon.
   */
  plug_location = (gpointer*)icon->priv;

  if (!plug_location || !*plug_location || !GTK_IS_PLUG (*plug_location))
    {
      g_warning ("Attempted to set up config window before status icon got"
                 "embedded !!!");
      return FALSE;
    }

  plug = GTK_PLUG (*plug_location);

  icon_win = gtk_plug_get_id (plug);

  XChangeProperty (GDK_DISPLAY(), icon_win,
                   tray_atom, XA_WINDOW,
                   32, PropModeReplace, (unsigned char*)&config_win, 1);

  XChangeProperty (GDK_DISPLAY(), icon_win,
                   type_atom, XA_STRING,
                   8, PropModeReplace,
                   (unsigned char*)tray_data->type,
                   strlen (tray_data->type));

  return TRUE;
}

static void
_mnbk_embedded_notify (GObject *gobject, GParamSpec *arg1, gpointer data)
{
  GtkStatusIcon *icon = GTK_STATUS_ICON (gobject);
  GtkPlug       *config = GTK_PLUG (data);

  if (gtk_status_icon_is_embedded (icon))
    {
      /*
       * When we get embedded, we need to set up the config window again,
       * as the releavent XID is different.
       */
      _mnbk_setup_config_window (icon, gtk_plug_get_id (config),
                                 &_mnbk_tray_setup_data);
    }
}

static gboolean
_mnbk_config_show_on_delete (GtkWidget *config, GdkEvent *event, gpointer data)
{
  /*
   * Ensure the config window will be visible the next time we need it.
   */
  gtk_widget_show_all (config);

  /*
   * Returning TRUE here stops the widget from getting destroyed.
   */
  return TRUE;
}

static void
_mnbk_initial_embedded_notify (GObject    *gobject,
                               GParamSpec *arg1,
                               gpointer    data)
{
  struct mnbk_tray_setup_data *tray_data = data;
  GtkStatusIcon               *icon      = tray_data->icon;
  GtkPlug                     *config    = tray_data->config;

  /*
   * We can only set up the config window after the status icon has been
   * embedded (we need an XID of the embedded plug).
   */
  if (gtk_status_icon_is_embedded (icon))
    {
      if (_mnbk_setup_config_window (icon, gtk_plug_get_id (config), tray_data))
        {
          /*
           * We connect to notify on the status embedded property; when this
           * changes to TRUE (after the status icon got un-embedded first), we
           * need to re-run the setup, because the XID of the window we are
           * hanging the MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW property on has
           * changed.
           */
          g_signal_connect (icon, "notify::embedded",
                            G_CALLBACK (_mnbk_embedded_notify), config);

          /*
           * If the application hides the config window, the tray manager will
           * remove it from it's container; this triggers the delete-event; we
           * want to (a) stop this event from destroying the plug (so we can
           * show it again) and (b) call _show_all() on the plug, to make sure
           * that when it is needed again it can be made visible by the socket.
           */
          g_signal_connect (config, "delete-event",
                            G_CALLBACK (_mnbk_config_show_on_delete), icon);

          g_signal_handler_disconnect (icon, tray_data->init_cb_id);
          tray_data->init_cb_id = 0;
        }
    }
}

/*
 * This is the funtion that the application will use to set itself up.
 */
static void
mnbk_system_tray_init (GtkStatusIcon *icon, GtkPlug *config,
                       const gchar *type)
{
  _mnbk_tray_setup_data.icon = icon;
  _mnbk_tray_setup_data.config = config;
  _mnbk_tray_setup_data.type = g_strdup (type);

  gtk_widget_show_all (GTK_WIDGET (config));
  gtk_status_icon_set_visible (icon, TRUE);

  _mnbk_tray_setup_data.init_cb_id =
    g_signal_connect (icon, "notify::embedded",
                      G_CALLBACK (_mnbk_initial_embedded_notify),
                      &_mnbk_tray_setup_data);
}

#endif

#endif
