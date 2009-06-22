/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-clutter.h */
/*
 * Copyright (c) 2009 Intel Corp.
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

#ifndef _MPL_PANEL_CLUTTER
#define _MPL_PANEL_CLUTTER

#include <clutter/clutter.h>

#include "mpl-panel-client.h"

G_BEGIN_DECLS

#define MPL_PANEL_CLUTTER_INIT(argc, argv)              \
if (CLUTTER_INIT_SUCCESS != clutter_init (argc, argv))  \
  {                                                     \
    g_error ("Unable to initialize Clutter.\n");        \
  }                                                     \

#define MPL_PANEL_CLUTTER_INIT_WITH_GTK(argc, argv)             \
  gtk_init (argc, argv);                                        \
  clutter_x11_set_display (gdk_display);                        \
  clutter_x11_disable_event_retrieval ();                       \
                                                                \
  if (CLUTTER_INIT_SUCCESS != clutter_init (argc, argv))        \
    {                                                           \
      g_error ("Unable to initialize Clutter.\n");              \
    }                                                           \

#define MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK(panel)          \
  {                                                             \
    Window xid;                                                 \
                                                                \
    xid = mpl_panel_client_get_xid (panel);                     \
                                                                \
    if (xid == None)                                            \
      g_error ("Panel not properly initialized");               \
                                                                \
    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID(xid);       \
                                                                \
  }                                                             \

#define MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID(xid)  \
  {                                                     \
    GdkFilterReturn                                     \
      gdk_to_clutter_event_pump__ (GdkXEvent *xevent,   \
                                   GdkEvent  *event,    \
                                   gpointer   data)     \
    {                                                   \
      XEvent *xev = (XEvent*)xevent;                    \
      Window  xid = GPOINTER_TO_INT (data);             \
                                                        \
      if (xev->xany.window != xid)                      \
        return GDK_FILTER_CONTINUE;                     \
                                                        \
      switch (clutter_x11_handle_event (xev))           \
        {                                               \
        default:                                        \
        case CLUTTER_X11_FILTER_CONTINUE:               \
          return GDK_FILTER_CONTINUE;                   \
        case CLUTTER_X11_FILTER_TRANSLATE:              \
          return GDK_FILTER_TRANSLATE;                  \
        case CLUTTER_X11_FILTER_REMOVE:                 \
          return GDK_FILTER_REMOVE;                     \
        }                                               \
    };                                                  \
                                                        \
    gdk_window_add_filter (NULL,                        \
                           gdk_to_clutter_event_pump__, \
                           GINT_TO_POINTER (xid));      \
  }                                                     \


#define MPL_TYPE_PANEL_CLUTTER mpl_panel_clutter_get_type()

#define MPL_PANEL_CLUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_PANEL_CLUTTER, MplPanelClutter))

#define MPL_PANEL_CLUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_PANEL_CLUTTER, MplPanelClutterClass))

#define MPL_IS_PANEL_CLUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_PANEL_CLUTTER))

#define MPL_IS_PANEL_CLUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_PANEL_CLUTTER))

#define MPL_PANEL_CLUTTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_PANEL_CLUTTER, MplPanelClutterClass))

typedef struct _MplPanelClutterPrivate MplPanelClutterPrivate;

typedef struct
{
  MplPanelClient parent;

  MplPanelClutterPrivate *priv;
} MplPanelClutter;

typedef struct
{
  MplPanelClientClass parent_class;

} MplPanelClutterClass;

GType mpl_panel_clutter_get_type (void);

MplPanelClient *mpl_panel_clutter_new   (const gchar *name,
                                         const gchar *tooltip,
                                         const gchar *stylesheet,
                                         const gchar *button_style,
                                         gboolean     with_toolbar_service);

ClutterActor *mpl_panel_clutter_get_stage (MplPanelClutter *panel);

void          mpl_panel_clutter_track_actor_height (MplPanelClutter *panel,
                                                    ClutterActor    *actor);

G_END_DECLS

#endif /* _MPL_PANEL_CLUTTER */

