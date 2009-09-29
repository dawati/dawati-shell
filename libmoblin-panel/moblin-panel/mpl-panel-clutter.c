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

#define MAX_SUPPORTED_XEMBED_VERSION   1

#define XEMBED_MAPPED          (1 << 0)

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

static void xembed_init (MplPanelClutter *panel);
static void xembed_set_win_info (Display *xdpy, Window xwin, int flags);

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
  Window        embedder;
  Atom          Atom_XEMBED;

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

  hints.min_width = width;
  hints.min_height = height;
  hints.flags = PMinSize;

  XSetWMNormalHints (xdpy, priv->xwindow, &hints);
  XResizeWindow (xdpy, priv->xwindow, width, height);

  clutter_actor_set_size (priv->stage, width, height);
  clutter_stage_ensure_viewport (CLUTTER_STAGE (priv->stage));
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

  client_class->set_size         = mpl_panel_clutter_set_size;
}

static void
mpl_panel_clutter_init (MplPanelClutter *self)
{
  MplPanelClutterPrivate *priv;

  priv = self->priv = MPL_PANEL_CLUTTER_GET_PRIVATE (self);
}

ClutterX11FilterReturn
mpl_panel_clutter_xevent_filter (XEvent *xev, ClutterEvent *cev, gpointer data)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (data)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();

  switch (xev->type)
    {
    case MapNotify:
      return CLUTTER_X11_FILTER_CONTINUE;
    case ClientMessage:
      if (xev->xclient.message_type == priv->Atom_XEMBED)
	{
	  switch (xev->xclient.data.l[1])
	    {
	    case XEMBED_EMBEDDED_NOTIFY:
              priv->embedder = xev->xclient.data.l[3];

              /* Map */
              clutter_actor_show (priv->stage);

              /* Signal the embedder we are mapped */
	      xembed_set_win_info (xdpy, priv->xwindow, XEMBED_MAPPED);
	      break;
	    case XEMBED_WINDOW_ACTIVATE:
	      break;
	    case XEMBED_WINDOW_DEACTIVATE:
	      break;
	    case XEMBED_FOCUS_IN:
	      break;
            default: ;
	    }

          return CLUTTER_X11_FILTER_REMOVE;
	}
    default: ;
    }

  return CLUTTER_X11_FILTER_CONTINUE;
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
  XSetWindowAttributes    attr;
  Display                *xdpy;
  gint                    screen;

  if (G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->constructed (self);

  /*
   * TODO
   *
   * create stage in an override redirect window.
   */

  xdpy   = clutter_x11_get_default_display ();
  screen = clutter_x11_get_default_screen ();

  attr.override_redirect = True;

  priv->xwindow = xwin = XCreateWindow (xdpy,
                                        RootWindow (xdpy, screen),
                                        0, 0, 100, 100, 0,
                                        CopyFromParent, InputOutput,
                                        CopyFromParent,
                                        CWOverrideRedirect, &attr);

  xembed_init (MPL_PANEL_CLUTTER (self));

  stage = clutter_stage_get_default ();

  clutter_x11_set_stage_foreign (CLUTTER_STAGE (stage), xwin);

  priv->stage = stage;

  XSelectInput (xdpy, xwin,
                StructureNotifyMask |
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                FocusChangeMask |
                ExposureMask |
                KeyPressMask | KeyReleaseMask |
                EnterWindowMask | LeaveWindowMask |
                PropertyChangeMask);

  clutter_x11_add_filter (mpl_panel_clutter_xevent_filter, self);

  g_object_set (self, "xid",
                clutter_x11_get_stage_window (CLUTTER_STAGE (stage)), NULL);

  /* Load a base style for widgets used in the Mpl panels */
  mpl_panel_clutter_load_base_style ();
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


/*
 * The XEmbed stuff based on Matchbox keyboard.
 */
static void
xembed_set_win_info (Display *xdpy, Window xwin, int flags)
{
   guint32 list[2];

   Atom atom_ATOM_XEMBED_INFO;

   atom_ATOM_XEMBED_INFO
     = XInternAtom(xdpy, "_XEMBED_INFO", False);


   list[0] = MAX_SUPPORTED_XEMBED_VERSION;
   list[1] = flags;
   XChangeProperty (xdpy,
		    xwin,
		    atom_ATOM_XEMBED_INFO,
		    atom_ATOM_XEMBED_INFO, 32,
		    PropModeReplace, (unsigned char *) list, 2);
}

static void
xembed_init (MplPanelClutter *panel)
{
  MplPanelClutterPrivate *priv = panel->priv;
  Display                *xdpy = clutter_x11_get_default_display ();

  priv->Atom_XEMBED = XInternAtom(xdpy, "_XEMBED", False);

  xembed_set_win_info (xdpy, priv->xwindow, 0);
}

#if 0
/*
 * TODO -- currently not needed, probalby remove.
 */
static Bool
xembed_send_message (MplPanelClutter *panel,
                     Window          *w,
                     long             message,
                     long             detail,
                     long             data1,
                     long             data2)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (data)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();
  XEvent                  ev;

  memset(&ev, 0, sizeof(ev));

  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = Atom_XEMBED;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime; /* FIXME: Is this correct */
  ev.xclient.data.l[1] = message;
  ev.xclient.data.l[2] = detail;
  ev.xclient.data.l[3] = data1;
  ev.xclient.data.l[4] = data2;

  clutter_x11_trap_x_errors ();

  XSendEvent(xdpy, w, False, NoEventMask, &ev);
  XSync(xdpy, False);

  if (clutter_x11_untrap_x_errors ())
    return False;

  return True;
}
#endif
