/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
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

#ifndef MOBLIN_NETBOOK_SYSTEM_TRAY_H
#define MOBLIN_NETBOOK_SYSTEM_TRAY_H

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#define MOBLIN_SYSTEM_TRAY_EVENT          "MOBLIN_SYSTEM_TRAY_EVENT"
#define MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW  "MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW"

/*
 * NB: this can be at most 20 bytes. If necessary the type, button and count
 *     fields could be collapsed further.
 */
typedef struct
{
  guint32 type;
  guint16 button;
  guint16 count;
  gint16  x;
  gint16  y;
  guint32 time;
  guint32 modifiers;
} MnbkTrayEventData;

#ifndef MOBLIN_SYSTEM_TRAY_FROM_PLUGIN
/*
 * Utility code to set up Tray code in the application. Simply call
 *
 * mnbk_system_tray_init (icon, config_plug);
 *
 * icon: GtkStatusIcon* instance.
 *
 * config_plug: GtkPlug* instance; this is the top level container that
 * holding the application configuration window. Unlike in the normal X
 * tray set up, this window is shown *by the netbook plugin*, not the
 * application, in response to click on the status icon.
 *
 * See tests/test-tray.c for an example.
 */
static gboolean
_mnbk_setup_config_window (GtkStatusIcon *icon, Window config_win)
{
  Atom tray_atom = gdk_x11_get_xatom_by_name (MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW);
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

  return TRUE;
}

struct mnbk_tray_setup_data
{
  GtkStatusIcon *icon;
  GtkPlug       *config;
};

static struct mnbk_tray_setup_data _mnbk_tray_setup_data;

static gboolean
_mnbk_idle_config_setup (gpointer data)
{
  struct mnbk_tray_setup_data *tray_data = data;
  GtkStatusIcon               *icon      = tray_data->icon;
  GtkPlug                     *config    = tray_data->config;

  if (gtk_status_icon_is_embedded (icon))
    {
      if (!GTK_WIDGET_REALIZED (config))
        {
          XSync (GDK_DISPLAY (), False);
          return TRUE;
        }

      if (_mnbk_setup_config_window (icon, gtk_plug_get_id (config)))
        return FALSE;
    }

  return TRUE;
}

static void
mnbk_system_tray_init (GtkStatusIcon *icon, GtkPlug *config)
{
  _mnbk_tray_setup_data.icon = icon;
  _mnbk_tray_setup_data.config = config;

  gtk_widget_show_all (GTK_WIDGET (config));
  gtk_status_icon_set_visible (icon, TRUE);

  g_idle_add (_mnbk_idle_config_setup, &_mnbk_tray_setup_data);
}

#else
/*
 * Compile time assert to ensure we do not try to pack more than we can
 * into the MnbkTrayData structure.
 */
int assert_tray_data_fits_ClientMessage__[sizeof(MnbkTrayEventData)<=20 ? 0:-1];
#endif

#endif
