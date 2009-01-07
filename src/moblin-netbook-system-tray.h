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
 * Client-side message filter which takes care of dispatching the appropriate
 * activate and popup-menu signals.
 *
 * Install in the client application as
 *
 *  gdk_add_client_message_filter (gdk_atom_intern (MOBLIN_SYSTEM_TRAY_EVENT,
 *                                                  FALSE),
 *				   mnbtk_client_message_handler, icon);
 *
 * Where icon is a GtkStatusIcon instance. The GtkStatusIcon signal handlers
 * need to use coords stored in mnbk_event_x and mnbk_event_y instead of
 * querying the status icon.
 *
 * See tests/test-tray.c for a simple example.
 */
static gint32 mnbk_event_x = 0;
static gint32 mnbk_event_y = 0;

static GdkFilterReturn
mnbtk_client_message_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  GtkStatusIcon       *icon = GTK_STATUS_ICON (data);
  XClientMessageEvent *xev = (XClientMessageEvent *) xevent;
  MnbkTrayEventData   *td  = (MnbkTrayEventData*)&xev->data;

#if 0
  g_debug ("Got click: \n"
	   "           button: %d\n"
	   "             type: %d\n"
	   "            count: %d\n"
	   "                x: %d\n"
	   "                y: %d\n"
	   "        modifiers: 0x%x\n\n",
	   td->button,
	   td->type,
	   td->count,
	   td->x,
	   td->y,
	   td->modifiers);
#endif

  if (td->type == GDK_BUTTON_PRESS)
    {
      mnbk_event_x = td->x;
      mnbk_event_y = td->y;

      switch (td->button)
	{
	case 1:
	  g_signal_emit_by_name (icon, "activate", 0);
	  break;
	case 3:
	  g_signal_emit_by_name (icon, "popup-menu", 0, td->button, td->time);
	  break;
	default:
	  break;
	}
    }
}

/*
 * Hangs the window id for a config window onto the xwindow of the icon.
 *
 * Returns FALSE on failure.
 */
static gboolean
mnbtk_setup_config_window (GtkStatusIcon *icon, Window config_win)
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

#else
/*
 * Compile time assert to ensure we do not try to pack more than we can
 * into the MnbkTrayData structure.
 */
int assert_tray_data_fits_ClientMessage__[sizeof(MnbkTrayEventData)<=20 ? 0:-1];
#endif

#endif
