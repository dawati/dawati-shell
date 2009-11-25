/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-clutter.c */
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

#include <clutter/x11/clutter-x11.h>
#include <nbtk/nbtk.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <string.h>

#include "mpl-panel-clutter.h"

#define MPL_X_ERROR_TRAP() clutter_x11_trap_x_errors()

#define MPL_X_ERROR_UNTRAP()                            \
  {                                                     \
    gint e__;                                           \
    if ((e__ = clutter_x11_untrap_x_errors ()))         \
      g_debug (G_STRLOC ": Cought X error %d", e__);    \
  }


G_DEFINE_TYPE (MplPanelClutter, mpl_panel_clutter, MPL_TYPE_PANEL_CLIENT)

#define MPL_PANEL_CLUTTER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_PANEL_CLUTTER, MplPanelClutterPrivate))

static void mpl_panel_clutter_constructed (GObject *self);

enum
{
  PROP_0,
};

struct _MplPanelClutterPrivate
{
  ClutterActor *stage;
  Window        xwindow;

  ClutterActor *tracked_actor;
  guint         height_notify_cb;
};

static void
mpl_panel_clutter_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_clutter_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_clutter_dispose (GObject *self)
{
  G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->dispose (self);
}

static void
mpl_panel_clutter_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->finalize (object);
}

static void
mpl_panel_clutter_set_size (MplPanelClient *self, guint width, guint height)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();
  XSizeHints              hints;
  MplPanelClientClass    *p_class;

  p_class = MPL_PANEL_CLIENT_CLASS (mpl_panel_clutter_parent_class);

  hints.min_width = width;
  hints.min_height = height;
  hints.flags = PMinSize;

  MPL_X_ERROR_TRAP ();

  XSetWMNormalHints (xdpy, priv->xwindow, &hints);
  XResizeWindow (xdpy, priv->xwindow, width, height);

  MPL_X_ERROR_UNTRAP ();

  clutter_actor_set_size (priv->stage, width, height);
  clutter_stage_ensure_viewport (CLUTTER_STAGE (priv->stage));

  if (p_class->set_size)
    p_class->set_size (self, width, height);
}

static void
mpl_panel_clutter_set_position (MplPanelClient *self, gint x, gint y)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();
  MplPanelClientClass    *p_class;

  p_class = MPL_PANEL_CLIENT_CLASS (mpl_panel_clutter_parent_class);

  MPL_X_ERROR_TRAP ();

  XMoveWindow (xdpy, priv->xwindow, x, y);

  MPL_X_ERROR_UNTRAP ();

  if (p_class->set_position)
    p_class->set_position (self, x, y);
}

static void
mpl_panel_clutter_show (MplPanelClient *self)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;

  clutter_actor_show (priv->stage);
}

static void
mpl_panel_clutter_hide (MplPanelClient *self)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;

  clutter_actor_hide (priv->stage);
}

static void
mpl_panel_clutter_class_init (MplPanelClutterClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  MplPanelClientClass *client_class = MPL_PANEL_CLIENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelClutterPrivate));

  object_class->get_property     = mpl_panel_clutter_get_property;
  object_class->set_property     = mpl_panel_clutter_set_property;
  object_class->dispose          = mpl_panel_clutter_dispose;
  object_class->finalize         = mpl_panel_clutter_finalize;
  object_class->constructed      = mpl_panel_clutter_constructed;

  client_class->set_position     = mpl_panel_clutter_set_position;
  client_class->set_size         = mpl_panel_clutter_set_size;
  client_class->show             = mpl_panel_clutter_show;
  client_class->hide             = mpl_panel_clutter_hide;
}

static void
mpl_panel_clutter_init (MplPanelClutter *self)
{
  MplPanelClutterPrivate *priv;

  priv = self->priv = MPL_PANEL_CLUTTER_GET_PRIVATE (self);
}

void
mpl_panel_clutter_load_base_style (void)
{
  static gboolean already_loaded = FALSE;

  if (!already_loaded)
    {
      /* Load in a base cache and a base style */
      nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                     NBTK_CACHE);
      nbtk_style_load_from_file (nbtk_style_get_default (),
                                 THEMEDIR "/theme.css", NULL);
      already_loaded = TRUE;
    }
}

static void
mpl_panel_clutter_constructed (GObject *self)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;
  ClutterActor           *stage = NULL;
  Window                  xwin = None;
  Display                *xdpy;
  gint32                  myint;
  Atom                    atom1, atom2;

  if (G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->constructed (self);

  stage = clutter_stage_get_default ();
  xdpy  = clutter_x11_get_default_display ();

  priv->stage = stage;

  clutter_actor_realize (stage);

  priv->xwindow = xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  g_object_set (self, "xid", xwin, NULL);

  MPL_X_ERROR_TRAP ();

  /*
   * Make dock, sticky and position at the correct place.
   */
   atom1 = XInternAtom(xdpy, "_NET_WM_WINDOW_TYPE", False);
   atom2 = XInternAtom(xdpy, "_NET_WM_WINDOW_TYPE_DOCK", False);

   XChangeProperty (xdpy,
                    xwin,
                    atom1,
                    XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) &atom2, 1);

   atom1 = XInternAtom(xdpy, "_NET_WM_DESKTOP", False);
   myint = -1;

   XChangeProperty (xdpy,
                    xwin,
                    atom1,
                    XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) &myint, 1);

   XSync (xdpy, False);

   MPL_X_ERROR_UNTRAP ();

  /* Load a base style for widgets used in the Mpl panels */
  mpl_panel_clutter_load_base_style ();

  clutter_actor_hide (priv->stage);
}

MplPanelClient *
mpl_panel_clutter_new (const gchar *name,
                       const gchar *tooltip,
                       const gchar *stylesheet,
                       const gchar *button_style,
                       gboolean     with_toolbar_service)
{
  MplPanelClient *panel = g_object_new (MPL_TYPE_PANEL_CLUTTER,
                                        "name",            name,
                                        "tooltip",         tooltip,
                                        "stylesheet",      stylesheet,
                                        "button-style",    button_style,
                                        "toolbar-service", with_toolbar_service,
                                        NULL);

  return panel;
}

ClutterActor *
mpl_panel_clutter_get_stage (MplPanelClutter *panel)
{
  return panel->priv->stage;
}

static void
mpl_panel_clutter_actor_height_notify_cb (GObject    *gobject,
                                          GParamSpec *pspec,
                                          gpointer    data)
{
  ClutterActor   *actor = CLUTTER_ACTOR (gobject);
  MplPanelClient *panel = MPL_PANEL_CLIENT (data);
  guint           height;

  height = (guint) clutter_actor_get_height (actor);
  mpl_panel_client_set_height_request (panel, height);
}

/*
 * Sets up the panel for dynamically matching its height to that of the
 * supplied actor (e.g., the top-level panel widget).
 *
 * Passing NULL for actor on a subsequent call we terminated the height
 * tracking.
 */
void
mpl_panel_clutter_track_actor_height (MplPanelClutter *panel,
                                      ClutterActor    *actor)
{
  MplPanelClutterPrivate *priv = panel->priv;

  if (priv->tracked_actor && priv->height_notify_cb)
    {
      g_signal_handler_disconnect (priv->tracked_actor,
                                   priv->height_notify_cb);

      priv->height_notify_cb = 0;
      priv->tracked_actor = NULL;
    }

  if (actor)
    {
      guint height;

      /*
       * Match the current height of the actor
       */
      height = (guint) clutter_actor_get_height (actor);
      mpl_panel_client_set_height_request (MPL_PANEL_CLIENT (panel), height);

      /*
       * Now watch for changes in height.
       */
      priv->tracked_actor = actor;

      priv->height_notify_cb =
        g_signal_connect (actor, "notify::height",
                          G_CALLBACK (mpl_panel_clutter_actor_height_notify_cb),
                          panel);
    }
}

void
mpl_panel_clutter_init_lib (gint *argc, gchar ***argv)
{
  if (CLUTTER_INIT_SUCCESS != clutter_init (argc, argv))
    {
      g_error ("Unable to initialize Clutter.\n");
    }
}

void
mpl_panel_clutter_init_with_gtk (gint *argc, gchar ***argv)
{
  gtk_init (argc, argv);
  clutter_x11_set_display (gdk_display);
  clutter_x11_disable_event_retrieval ();

  if (CLUTTER_INIT_SUCCESS != clutter_init (argc, argv))
    {
      g_error ("Unable to initialize Clutter.\n");
    }

  mpl_panel_clutter_load_base_style ();
}

void
mpl_panel_clutter_setup_events_with_gtk_for_xid (Window xid)
{
  GdkFilterReturn
    gdk_to_clutter_event_pump__ (GdkXEvent *xevent,
                                 GdkEvent  *event,
                                 gpointer   data)
  {
    static GdkWindow *gdk_win = NULL;

    XEvent *xev = (XEvent*)xevent;
    guint32 timestamp = 0;

    if (!gdk_win)
      gdk_win = gdk_window_foreign_new (GPOINTER_TO_INT (data));

    /*
     * Ensure we update the user time on this window if the event implies user
     * action.
     */
    switch (event->type)
      {
      case GDK_BUTTON_PRESS:
      case GDK_2BUTTON_PRESS:
      case GDK_3BUTTON_PRESS:
      case GDK_BUTTON_RELEASE:
        timestamp = event->button.time;
        break;
      case GDK_MOTION_NOTIFY:
        timestamp = event->motion.time;
        break;
      case GDK_KEY_PRESS:
      case GDK_KEY_RELEASE:
        timestamp = event->key.time;
        break;
      default: ;
      }

    if (timestamp && gdk_win)
      gdk_x11_window_set_user_time (gdk_win, timestamp);

    switch (clutter_x11_handle_event (xev))
      {
      default:
      case CLUTTER_X11_FILTER_CONTINUE:
        return GDK_FILTER_CONTINUE;
      case CLUTTER_X11_FILTER_TRANSLATE:
        return GDK_FILTER_TRANSLATE;
      case CLUTTER_X11_FILTER_REMOVE:
        return GDK_FILTER_REMOVE;
      }
  };

  XSelectInput (GDK_DISPLAY (), xid,
                StructureNotifyMask |
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                FocusChangeMask |
                ExposureMask |
                KeyPressMask | KeyReleaseMask |
                EnterWindowMask | LeaveWindowMask |
                PropertyChangeMask);

  gdk_window_add_filter (NULL,
                         gdk_to_clutter_event_pump__,
                         GINT_TO_POINTER (xid));
}

void
mpl_panel_clutter_setup_events_with_gtk (MplPanelClient *panel)
{
    Window xid;

    xid = mpl_panel_client_get_xid (panel);

    if (xid == None)
      g_error ("Panel not properly initialized");

    mpl_panel_clutter_setup_events_with_gtk_for_xid (xid);

    mpl_panel_clutter_load_base_style ();
}
