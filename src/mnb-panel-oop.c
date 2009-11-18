/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel.c */
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

#include "mnb-panel-oop.h"
#include "mnb-toolbar.h"
#include "marshal.h"

#include <string.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <moblin-panel/mpl-panel-common.h>
#include <display.h>
#include <errors.h>

/*
 * Including mutter's errors.h defines the i18n macros, so undefine them before
 * including the glib i18n header.
 */
#undef _
#undef N_

/* FIME -- duplicated from MnbDropDown.c */
#define SLIDE_DURATION 150

#include <X11/Xatom.h>

static void mnb_panel_iface_init (MnbPanelIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbPanelOop,
                         mnb_panel_oop,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MNB_TYPE_PANEL,
                                                mnb_panel_iface_init));

#define MNB_PANEL_OOP_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL_OOP, MnbPanelOopPrivate))

static void     mnb_panel_oop_constructed    (GObject  *self);
static gboolean mnb_panel_oop_setup_proxy    (MnbPanelOop *panel);
static void     mnb_panel_oop_init_owner     (MnbPanelOop *panel);
static void     mnb_panel_oop_dbus_proxy_weak_notify_cb (gpointer, GObject *);

static const gchar * mnb_panel_oop_get_name (MnbPanel *panel);
static const gchar * mnb_panel_oop_get_tooltip (MnbPanel *panel);
static const gchar * mnb_panel_oop_get_button_style (MnbPanel *panel);
static const gchar  *mnb_panel_oop_get_stylesheet    (MnbPanel *panel);
static void mnb_panel_oop_set_size (MnbPanel *panel, guint width, guint height);
static void mnb_panel_oop_show (MnbPanel *panel);

enum
{
  PROP_0,

  PROP_DBUS_NAME,
  PROP_WIDTH,
  PROP_HEIGHT,
};

enum
{
  READY,
  REQUEST_BUTTON_STYLE,
  REQUEST_TOOLTIP,
  REMOTE_PROCESS_DIED,
  SET_SIZE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MnbPanelOopPrivate
{
  DBusGConnection *dbus_conn;
  DBusGProxy      *proxy;
  gchar           *dbus_name;

  gchar           *name;
  gchar           *tooltip;
  gchar           *stylesheet;
  gchar           *button_style_id;
  guint            xid;
  guint            child_xid;
  gchar           *child_class;

  GtkWidget       *window;

  guint            width;
  guint            height;

  MutterWindow    *mcw;

  gboolean         constructed      : 1;
  gboolean         dead             : 1; /* Set when the remote  */
  gboolean         ready            : 1;
  gboolean         hide_in_progress : 1;

  /*
   * The show/hide machinery
   */
  gboolean         in_show_animation : 1;
  gboolean         in_hide_animation : 1;
  gboolean         hide_toolbar      : 1;

  NbtkButton      *button;

  gulong           show_completed_id;
  gulong           hide_completed_id;
  ClutterAnimation *show_anim;
  ClutterAnimation *hide_anim;

};

static void
mnb_panel_oop_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (object)->priv;

  switch (property_id)
    {
    case PROP_DBUS_NAME:
      g_value_set_string (value, priv->dbus_name);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, priv->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_oop_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (object)->priv;

  switch (property_id)
    {
    case PROP_DBUS_NAME:
      g_free (priv->dbus_name);
      priv->dbus_name = g_value_dup_string (value);
      break;
    case PROP_WIDTH:
      priv->width = g_value_get_uint (value);
      break;
    case PROP_HEIGHT:
      priv->height = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

void
mnb_panel_oop_focus (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;
  MetaDisplay        *display;
  MutterPlugin       *plugin = moblin_netbook_get_plugin_singleton ();
  guint32             timestamp;

  display   = meta_screen_get_display (mutter_plugin_get_screen (plugin));
  timestamp = meta_display_get_current_time_roundtrip (display);

  meta_error_trap_push (display);

  XRaiseWindow (meta_display_get_xdisplay (display), priv->xid);
  XSync (meta_display_get_xdisplay (display), False);

  if (priv->mcw)
    meta_display_set_input_focus_window (display,
                                     mutter_window_get_meta_window (priv->mcw),
                                     FALSE,
                                     timestamp);

  XSetInputFocus (meta_display_get_xdisplay (display), priv->child_xid,
                  RevertToPointerRoot, timestamp);

  meta_error_trap_pop (display, TRUE);
}

/*
 * Callbacks for the request signals exposed by the panels.
 */
static void
mnb_panel_oop_request_focus_cb (DBusGProxy *proxy, MnbPanelOop *panel)
{
  if (!CLUTTER_ACTOR_IS_MAPPED (panel))
    {
      g_warning ("Panel %s requested focus while not visible !!!",
                 mnb_panel_oop_get_name ((MnbPanel*)panel));
      return;
    }

  mnb_panel_oop_focus (panel);
}

static void
mnb_panel_oop_request_show_cb (DBusGProxy *proxy, MnbPanelOop *panel)
{
  clutter_actor_show (CLUTTER_ACTOR (panel));
}

static void
mnb_panel_oop_request_hide_cb (DBusGProxy *proxy, MnbPanelOop *panel)
{
  mnb_panel_hide_with_toolbar ((MnbPanel*)panel);
}

static void
mnb_panel_oop_request_button_style_cb (DBusGProxy  *proxy,
                                       const gchar *style_id,
                                       MnbPanelOop    *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  g_free (priv->button_style_id);
  priv->button_style_id = g_strdup (style_id);

  g_signal_emit (panel, signals[REQUEST_BUTTON_STYLE], 0, style_id);
}

static void
mnb_panel_oop_request_tooltip_cb (DBusGProxy  *proxy,
                                  const gchar *tooltip,
                                  MnbPanelOop    *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  g_free (priv->tooltip);
  priv->tooltip = g_strdup (tooltip);

  g_signal_emit (panel, signals[REQUEST_TOOLTIP], 0, tooltip);
}

static void
mnb_panel_oop_set_size_cb (DBusGProxy *proxy,
                           guint       width,
                           guint       height,
                           MnbPanelOop   *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;
  GtkWidget          *socket;
  GdkWindow          *socket_win;

  if (!priv->window)
    return;

  priv->width  = width;
  priv->height = height;

  /*
   * Resize our top-level window to match. If the window is hidden, move it
   * so that we can be sure it is entirely off screen.
   */
  if (!CLUTTER_ACTOR_IS_VISIBLE (panel))
    gtk_window_move (GTK_WINDOW (priv->window),
                     TOOLBAR_X_PADDING, -(height + 100));

  gtk_window_resize (GTK_WINDOW (priv->window), width, height);

  if ((socket = gtk_bin_get_child (GTK_BIN (priv->window))) &&
      (socket_win = gtk_socket_get_plug_window (GTK_SOCKET (socket))))
    gdk_window_resize (socket_win, width, height);

  g_signal_emit (panel, signals[SET_SIZE], 0, width, height);
}

static void
mnb_panel_oop_dispose (GObject *self)
{
  MnbPanelOopPrivate *priv   = MNB_PANEL_OOP (self)->priv;
  DBusGProxy         *proxy  = priv->proxy;
  GtkWidget          *window = priv->window;

  if (priv->window)
    {
      priv->window = NULL;
      gtk_widget_destroy (window);
    }

  if (proxy)
    {
      g_object_weak_unref (G_OBJECT (proxy),
                           mnb_panel_oop_dbus_proxy_weak_notify_cb, self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestFocus",
                                   G_CALLBACK (mnb_panel_oop_request_focus_cb),
                                   self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestButtonStyle",
                             G_CALLBACK (mnb_panel_oop_request_button_style_cb),
                             self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestTooltip",
                                  G_CALLBACK (mnb_panel_oop_request_tooltip_cb),
                                  self);
      g_object_unref (proxy);
      priv->proxy = NULL;
    }

  if (priv->dbus_conn)
    {
      dbus_g_connection_unref (priv->dbus_conn);
      priv->dbus_conn = NULL;
    }

  G_OBJECT_CLASS (mnb_panel_oop_parent_class)->dispose (self);
}

static void
mnb_panel_oop_finalize (GObject *object)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (object)->priv;

  g_free (priv->dbus_name);

  g_free (priv->name);
  g_free (priv->tooltip);
  g_free (priv->stylesheet);
  g_free (priv->button_style_id);
  g_free (priv->child_class);

  G_OBJECT_CLASS (mnb_panel_oop_parent_class)->finalize (object);
}

#include "mnb-panel-dbus-bindings.h"

static void
mnb_panel_oop_class_init (MnbPanelOopClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelOopPrivate));

  object_class->get_property     = mnb_panel_oop_get_property;
  object_class->set_property     = mnb_panel_oop_set_property;
  object_class->dispose          = mnb_panel_oop_dispose;
  object_class->finalize         = mnb_panel_oop_finalize;
  object_class->constructed      = mnb_panel_oop_constructed;

  g_object_class_install_property (object_class,
                                   PROP_DBUS_NAME,
                                   g_param_spec_string ("dbus-name",
                                                        "Dbus name",
                                                        "Dbus name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_uint ("width",
                                                      "Width",
                                                      "Width",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_HEIGHT,
                                   g_param_spec_uint ("height",
                                                      "Height",
                                                      "Height",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  signals[REQUEST_BUTTON_STYLE] =
    g_signal_new ("request-button-style",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelOopClass, request_button_style),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[REQUEST_TOOLTIP] =
    g_signal_new ("request-tooltip",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelOopClass, request_tooltip),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[READY] =
    g_signal_new ("ready",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelOopClass, ready),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REMOTE_PROCESS_DIED] =
    g_signal_new ("remote-process-died",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelOopClass, remote_process_died),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[SET_SIZE] =
    g_signal_new ("set-size",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelOopClass, set_size),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT,
                  G_TYPE_UINT);

  dbus_g_object_register_marshaller (moblin_netbook_marshal_VOID__UINT_UINT,
                                     G_TYPE_NONE,
                                     G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
}

static void
mnb_panel_oop_init (MnbPanelOop *self)
{
  MnbPanelOopPrivate *priv;

  priv = self->priv = MNB_PANEL_OOP_GET_PRIVATE (self);
}

/*
 * It is not possible to pass a NULL for the reply function into the
 * autogenerated bindings, as they do not check :/
 */
static void
mnb_panel_oop_dbus_dumb_reply_cb (DBusGProxy *proxy,
                                  GError     *error,
                                  gpointer    data)
{
}

/*
 * Signal closures for show/hide related signals; we translate these into the
 * appropriate dbus method calls.
 */
static void
mnb_panel_oop_show_begin (MnbPanel *self)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (self)->priv;

  org_moblin_UX_Shell_Panel_show_begin_async (priv->proxy,
                                              mnb_panel_oop_dbus_dumb_reply_cb,
                                              NULL);
}

static void
mnb_panel_oop_show_completed (MnbPanel *self)
{
  MnbPanelOopPrivate *priv  = MNB_PANEL_OOP (self)->priv;
  gfloat           x = 0, y = 0;

  clutter_actor_get_position (CLUTTER_ACTOR (self), &x, &y);

  gtk_window_move (GTK_WINDOW (priv->window),
                   (gint)x + TOOLBAR_X_PADDING,
                   (gint)y + MNB_DROP_DOWN_TOP_PADDING);

  mnb_panel_oop_focus (MNB_PANEL_OOP (self));

  org_moblin_UX_Shell_Panel_show_end_async (priv->proxy,
                                            mnb_panel_oop_dbus_dumb_reply_cb,
                                            NULL);
}

static void
mnb_panel_oop_hide_begin (MnbPanel *self)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (self)->priv;
  GtkWidget          *window = priv->window;

  priv->hide_in_progress = TRUE;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

  /*
   * We do not hide the window here, only move it off screen; this significantly
   * improves the Toolbar button response time.
   */
  gtk_window_move (GTK_WINDOW (window), TOOLBAR_X_PADDING,
                   -(priv->height + 100));

  org_moblin_UX_Shell_Panel_hide_begin_async (priv->proxy,
                                              mnb_panel_oop_dbus_dumb_reply_cb,
                                              NULL);
}

static void
mnb_panel_oop_hide_completed (MnbPanel *self)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (self)->priv;

  priv->hide_in_progress = FALSE;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

  if (priv->window)
    {
      gtk_widget_hide (priv->window);
    }

  org_moblin_UX_Shell_Panel_hide_end_async (priv->proxy,
                                            mnb_panel_oop_dbus_dumb_reply_cb,
                                            NULL);
}

static DBusGConnection *
mnb_panel_oop_connect_to_dbus ()
{
  DBusGConnection *conn;
  GError          *error = NULL;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (!conn)
    {
      g_warning ("Cannot connect to DBus: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  return conn;
}

static void
mnb_panel_oop_socket_size_allocate_cb (GtkWidget     *widget,
                                       GtkAllocation *allocation,
                                       gpointer       data)
{
  GtkSocket *socket = GTK_SOCKET (widget);

  if (socket->is_mapped)
    {
      MnbPanelOopPrivate *priv = MNB_PANEL_OOP (data)->priv;

      /*
       * If the window is mapped, and we are not currently in the hide
       * animation, ensure the window position.
       */
      if (!priv->hide_in_progress && CLUTTER_ACTOR_IS_MAPPED (data))
        {
          gfloat x = 0, y = 0;

          clutter_actor_get_position (CLUTTER_ACTOR (data), &x, &y);

          gtk_window_move (GTK_WINDOW (priv->window),
                           (gint)x + TOOLBAR_X_PADDING,
                           (gint)y + MNB_DROP_DOWN_TOP_PADDING);
        }
    }
}

static void
mnb_panel_oop_dbus_proxy_weak_notify_cb (gpointer data, GObject *object)
{
  MnbPanelOop        *panel = MNB_PANEL_OOP (data);
  MnbPanelOopPrivate *priv  = panel->priv;

  priv->proxy = NULL;

  g_object_ref (panel);
  priv->ready = FALSE;
  priv->dead = TRUE;
  g_signal_emit (panel, signals[REMOTE_PROCESS_DIED], 0);
  g_object_unref (panel);
}

static gboolean
mnb_panel_oop_plug_removed_cb (GtkSocket *socket, gpointer data)
{
  MnbPanelOop        *panel = MNB_PANEL_OOP (data);
  MnbPanelOopPrivate *priv  = panel->priv;

  priv->child_xid = None;

  g_object_ref (panel);
  priv->ready = FALSE;
  priv->dead = TRUE;
  g_signal_emit (panel, signals[REMOTE_PROCESS_DIED], 0);
  g_object_unref (panel);

  return FALSE;
}

static void
mnb_panel_oop_init_panel_oop_reply_cb (DBusGProxy *proxy,
                                       gchar      *name,
                                       guint       xid,
                                       gchar      *tooltip,
                                       gchar      *stylesheet,
                                       gchar      *button_style_id,
                                       guint       window_width,
                                       guint       window_height,
                                       GError     *error,
                                       gpointer    panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;
  GtkWidget       *socket;
  GtkWidget       *window;

  if (error)
    {
      g_warning ("Could not initialize Panel %s: %s",
                 mnb_panel_oop_get_name ((MnbPanel*)panel), error->message);
      clutter_actor_destroy (CLUTTER_ACTOR (panel));
      return;
    }

  /*
   * We duplicate the return values, because we need to be able to replace them
   * and using the originals we would need to use dbus_malloc() later on
   * to set them afresh.
   */
  g_free (priv->name);
  priv->name = g_strdup (name);

  g_free (priv->tooltip);
  priv->tooltip = g_strdup (tooltip);

  g_free (priv->stylesheet);
  priv->stylesheet = g_strdup (stylesheet);

  g_free (priv->button_style_id);
  priv->button_style_id = g_strdup (button_style_id);

  priv->child_xid = xid;

  g_free (priv->child_class);

  /*
   * Retrieve the WM_CLASS property for the child window (we have to do it the
   * hard way, because the WM_CLASS on the MutterWindow is coming from mutter,
   * not the application).
   *
   * (We use the wm-class to identify sub-windows.)
   */
  {
    Atom r_type;
    int  r_fmt;
    unsigned long n_items;
    unsigned long r_after;
    char *r_prop;
    MutterPlugin *plugin = moblin_netbook_get_plugin_singleton ();
    MetaDisplay *display;

    display = meta_screen_get_display (mutter_plugin_get_screen (plugin));

    meta_error_trap_push (display);

    if (Success == XGetWindowProperty (GDK_DISPLAY (), xid, XA_WM_CLASS,
                                       0, 8192,
                                       False, XA_STRING,
                                       &r_type, &r_fmt, &n_items, &r_after,
                                       (unsigned char **)&r_prop) &&
        r_type != 0)
      {
        if (r_prop)
          {
            /*
             * The property contains two strings separated by \0; we want the
             * second string.
             */
            gint len0 = strlen (r_prop);

            if (len0 == n_items)
              len0--;

            priv->child_class = g_strdup (r_prop + len0 + 1);

            g_debug ("child class %s", priv->child_class);
            XFree (r_prop);
          }
      }

    meta_error_trap_pop (display, TRUE);
  }

  /*
   * If we already have a window, we are being called because the panel has
   * died on us; we simply destroy the window and start again. (It should be
   * possible to just destroy the socket and reuse the window (after unmapping
   * it, but this is simple and robust.)
   */
  if (priv->window)
    {
      gtk_widget_destroy (priv->window);
    }

  socket = gtk_socket_new ();
  priv->window = window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);
#if 0
  gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
#endif
  gtk_window_resize (GTK_WINDOW (window), window_width, window_height);

  gtk_window_move (GTK_WINDOW (window),
                   TOOLBAR_X_PADDING,
                   TOOLBAR_HEIGHT + MNB_DROP_DOWN_TOP_PADDING);

  gtk_container_add (GTK_CONTAINER (window), socket);
  gtk_widget_realize (socket);
  gtk_socket_add_id (GTK_SOCKET (socket), xid);

  g_signal_connect (socket, "size-allocate",
                    G_CALLBACK (mnb_panel_oop_socket_size_allocate_cb), panel);

  g_signal_connect (socket, "plug-removed",
                    G_CALLBACK (mnb_panel_oop_plug_removed_cb), panel);

  gtk_widget_realize (window);

  priv->xid = GDK_WINDOW_XWINDOW (window->window);

  priv->ready = TRUE;
  priv->dead = FALSE;

  dbus_free (name);
  dbus_free (tooltip);
  dbus_free (stylesheet);
  dbus_free (button_style_id);

  g_signal_emit (panel, signals[READY], 0);
}

/*
 * Does the hard work in establishing a connection to the name owner, retrieving
 * the require information, and constructing the socket into which we embed the
 * panel window.
 */
static void
mnb_panel_oop_init_owner (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

  /*
   * Now call the remote init_panel_oop() method to obtain the panel name,
   * tooltip and xid.
   */
  org_moblin_UX_Shell_Panel_init_panel_async (priv->proxy,
                                          priv->width, priv->height,
                                          mnb_panel_oop_init_panel_oop_reply_cb,
                                          panel);
}

static void
mnb_panel_oop_proxy_owner_changed_cb (DBusGProxy *proxy,
                                      const char *name,
                                      const char *old,
                                      const char *new,
                                      gpointer    data)
{
  mnb_panel_oop_init_owner (MNB_PANEL_OOP (data));
}

DBusGProxy *
mnb_panel_oop_create_dbus_proxy (DBusGConnection *dbus_conn,
                                 const gchar     *dbus_name)
{
  DBusGProxy *proxy;
  gchar      *dbus_path;
  gchar      *p;

  dbus_path = g_strconcat ("/", dbus_name, NULL);

  p = dbus_path;
  while (*p)
    {
      if (*p == '.')
        *p = '/';

      ++p;
    }

  proxy = dbus_g_proxy_new_for_name (dbus_conn,
                                     dbus_name,
                                     dbus_path,
                                     MPL_PANEL_DBUS_INTERFACE);

  g_free (dbus_path);

  return proxy;
}

static gboolean
mnb_panel_oop_setup_proxy (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;
  DBusGProxy         *proxy;

  if (!priv->dbus_conn)
    {
      g_warning (G_STRLOC " No dbus connection, cannot connect to panel!");
      return FALSE;
    }

  proxy = mnb_panel_oop_create_dbus_proxy (priv->dbus_conn, priv->dbus_name);

  if (!proxy)
    {
      g_warning ("Unable to create proxy for %s (reason unknown)",
                 priv->dbus_name);

      return FALSE;
    }

  priv->proxy = proxy;

  g_object_weak_ref (G_OBJECT (proxy),
                     mnb_panel_oop_dbus_proxy_weak_notify_cb, panel);

  /*
   * Hook up to the signals the interface defines.
   */
  dbus_g_proxy_add_signal (proxy, "RequestShow", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestShow",
                               G_CALLBACK (mnb_panel_oop_request_show_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestHide", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestHide",
                               G_CALLBACK (mnb_panel_oop_request_hide_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestFocus", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestFocus",
                               G_CALLBACK (mnb_panel_oop_request_focus_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged",
                             G_CALLBACK (mnb_panel_oop_proxy_owner_changed_cb),
                             panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestButtonStyle",
                           G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestButtonStyle",
                             G_CALLBACK (mnb_panel_oop_request_button_style_cb),
                             panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestTooltip",
                           G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestTooltip",
                               G_CALLBACK (mnb_panel_oop_request_tooltip_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "SetSize",
                           G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "SetSize",
                               G_CALLBACK (mnb_panel_oop_set_size_cb),
                               panel, NULL);

  mnb_panel_oop_init_owner (panel);

  return TRUE;
}

static void
mnb_panel_oop_constructed (GObject *self)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (self)->priv;
  DBusGConnection *conn;

  /*
   * Make sure our parent gets chance to do what it needs to.
   */
  if (G_OBJECT_CLASS (mnb_panel_oop_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_panel_oop_parent_class)->constructed (self);

  if (!priv->dbus_name)
    return;

  conn = mnb_panel_oop_connect_to_dbus ();

  if (!conn)
    {
      g_warning (G_STRLOC " Unable to connect to DBus!");
      return;
    }

  priv->dbus_conn = conn;

  if (!mnb_panel_oop_setup_proxy (MNB_PANEL_OOP (self)))
    return;

  priv->constructed = TRUE;
}

MnbPanelOop *
mnb_panel_oop_new (MutterPlugin *plugin,
               const gchar  *dbus_name,
               guint         width,
               guint         height)
{
  MnbPanelOop *panel = g_object_new (MNB_TYPE_PANEL_OOP,
                                  "mutter-plugin", plugin,
                                  "dbus-name",     dbus_name,
                                  "width",         width,
                                  "height",        height,
                                  NULL);

  if (!panel->priv->constructed)
    {
      g_warning (G_STRLOC " Construction of Panel for %s failed.", dbus_name);

      clutter_actor_destroy (CLUTTER_ACTOR (panel));
      return NULL;
    }

  return panel;
}

static const gchar *
mnb_panel_oop_get_name (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;

  return priv->name;
}

const gchar *
mnb_panel_oop_get_dbus_name (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  return priv->dbus_name;
}

static const gchar *
mnb_panel_oop_get_tooltip (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;

  return priv->tooltip;
}

static const gchar *
mnb_panel_oop_get_stylesheet (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;

  return priv->stylesheet;
}

static const gchar *
mnb_panel_oop_get_button_style (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;

  return priv->button_style_id;
}

static void
mnb_panel_oop_mutter_window_destroy_cb (ClutterActor *actor, gpointer data)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (data)->priv;

  priv->mcw = NULL;

  clutter_actor_hide (CLUTTER_ACTOR (data));
}

static void
mnb_panel_oop_pixmap_size_notify_cb (GObject    *gobject,
                                     GParamSpec *pspec,
                                     gpointer    data)
{
  clutter_actor_queue_relayout (CLUTTER_ACTOR (data));
}

/*
 * FIXME
 */
void
mnb_panel_oop_show_mutter_window (MnbPanelOop *panel, MutterWindow *mcw)
{
  MnbPanelOopPrivate *priv    = panel->priv;
  ClutterActor    *texture;

  if (!mcw)
    {
      if (priv->mcw)
        {
          g_signal_handlers_disconnect_by_func (priv->mcw,
                                        mnb_panel_oop_mutter_window_destroy_cb,
                                        panel);

          texture = mutter_window_get_texture (priv->mcw);

          if (texture)
            g_signal_handlers_disconnect_by_func (texture,
                                           mnb_panel_oop_pixmap_size_notify_cb,
                                           panel);

          priv->mcw = NULL;
        }

      mnb_drop_down_set_child (MNB_DROP_DOWN (panel), NULL);

      return;
    }

  if (mcw == priv->mcw)
    return;

  texture = mutter_window_get_texture (mcw);

  priv->mcw = mcw;

  g_object_ref (texture);
  clutter_actor_unparent (texture);
  mnb_drop_down_set_child (MNB_DROP_DOWN (panel), texture);

  g_signal_connect (texture, "notify::pixmap-width",
                    G_CALLBACK (mnb_panel_oop_pixmap_size_notify_cb),
                    panel);
  g_signal_connect (texture, "notify::pixmap-height",
                    G_CALLBACK (mnb_panel_oop_pixmap_size_notify_cb),
                    panel);

  g_object_unref (texture);

  g_signal_connect (mcw, "destroy",
                    G_CALLBACK (mnb_panel_oop_mutter_window_destroy_cb),
                    panel);

  g_object_set (mcw, "no-shadow", TRUE, NULL);

  clutter_actor_hide (CLUTTER_ACTOR (mcw));

  CLUTTER_ACTOR_CLASS (mnb_panel_oop_parent_class)->show (CLUTTER_ACTOR (panel));
}

guint
mnb_panel_oop_get_xid (MnbPanelOop *panel)
{
  return panel->priv->xid;
}

gboolean
mnb_panel_oop_is_ready (MnbPanelOop *panel)
{
  return panel->priv->ready;
}

/*
 * FIXME !!!
 */
static void
mnb_panel_oop_set_size (MnbPanel *panel, guint width, guint height)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;
  gfloat x, y, w, h;
  gint   wi, hi, hfi;
  gboolean h_change = FALSE, w_change = FALSE;

  /*
   * Exit if no change
   */
  clutter_actor_get_size (CLUTTER_ACTOR (panel), &w, &h);

  if ((guint)w != width)
    w_change = TRUE;

  if ((guint)h != height)
    h_change = TRUE;

  if (!w_change && !h_change)
    return;

  mnb_drop_down_get_footer_geometry (MNB_DROP_DOWN (panel), &x, &y, &w, &h);

  /*
   * Hack: the footer geometry API for some reason returns 0 the first time
   * we try to show the panel. Probably the panel is not yet fully setup. As
   * a quick fix, fallback to the default size here, if that happens.
   */
  hfi = (gint) h;

  if (!hfi)
    hfi = TOOLBAR_HEIGHT/2;

  /*
   * FIXME -- the border and shaddow should not be hardcoded here.
   */
  wi = width - 2 * 4;
  hi = height - hfi - MNB_DROP_DOWN_TOP_PADDING - MNB_DROP_DOWN_SHADOW_HEIGHT;

  if (wi < 0)
    wi = 0;

  if (hi < 0)
    hi = 0;

  org_moblin_UX_Shell_Panel_set_size_async (priv->proxy, wi, hi,
                                            mnb_panel_oop_dbus_dumb_reply_cb,
                                            NULL);
}

MutterWindow *
mnb_panel_oop_get_mutter_window (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  return priv->mcw;
}

/*
 * Retruns TRUE if the passed window is both distinct from the panel window,
 * and belongs to the same window class.
 */
gboolean
mnb_panel_oop_owns_window (MnbPanelOop *panel, MutterWindow *mcw)
{
  MnbPanelOopPrivate *priv = panel->priv;
  const gchar        *wclass;

  if (!priv->mcw || !mcw)
    return FALSE;

  /*
   * Return FALSE for the top level panel window.
   */
  if (priv->mcw == mcw)
    return FALSE;

  wclass = meta_window_get_wm_class (mutter_window_get_meta_window (mcw));

  if (priv->child_class && wclass && !strcmp (priv->child_class, wclass))
    return TRUE;

  return FALSE;
}

/*
 * Returns TRUE if the passed window is both distinct from the panel window,
 * and transient for it.
 */
gboolean
mnb_panel_oop_is_ancestor_of_transient (MnbPanelOop *panel, MutterWindow *mcw)
{
  MutterWindow *pcw;
  MetaWindow   *pmw, *mw;

  if (!panel)
    return FALSE;

  pcw = mnb_panel_oop_get_mutter_window (panel);

  if (!pcw || pcw == mcw)
    return FALSE;

  pmw = mutter_window_get_meta_window (pcw);
  mw  = mutter_window_get_meta_window (mcw);

  return meta_window_is_ancestor_of_transient (pmw, mw);
}

static void
mnb_panel_oop_dbus_ping_cb (DBusGProxy *proxy, GError *error, gpointer data)
{
  g_debug ("Ping to %s acknowledged", (gchar*)data);
  g_free (data);
}

void
mnb_toolbar_ping_panel_oop (DBusGConnection *dbus_conn, const gchar *dbus_name)
{
  DBusGProxy *proxy = mnb_panel_oop_create_dbus_proxy (dbus_conn, dbus_name);

  if (!proxy)
    {
      g_warning ("Unable to create proxy for %s (reason unknown)", dbus_name);
      return;
    }

  org_moblin_UX_Shell_Panel_ping_async (proxy,
                                        mnb_panel_oop_dbus_ping_cb,
                                        g_strdup (dbus_name));

  g_object_unref (proxy);
}

static void
mnb_panel_oop_show_completed_cb (ClutterAnimation *anim, MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  priv->in_show_animation = FALSE;
  priv->hide_toolbar = FALSE;
  priv->show_anim = NULL;
  priv->show_completed_id = 0;

  if (priv->button)
    {
      if (!nbtk_button_get_checked (priv->button))
        nbtk_button_set_checked (priv->button, TRUE);
    }

  g_signal_emit_by_name (panel, "show-completed");
  g_object_unref (priv->mcw);
}

static void
mnb_toolbar_show_completed_cb (MnbToolbar *toolbar, gpointer data)
{
  MnbPanelOop *panel = data;

  g_signal_handlers_disconnect_by_func (toolbar,
                                        mnb_toolbar_show_completed_cb,
                                        data);

  mnb_panel_oop_show ((MnbPanel*)panel);
}

/*
 * Ensure that the drop down does not stretch over any reserved strut at the
 * bottom of the screen (e.g., if VKB is present)
 */
static void
mnb_panel_oop_ensure_size (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv  = panel->priv;
  ClutterActor    *mcw = (ClutterActor*)priv->mcw;

  if (mcw)
    {
      MetaRectangle  r;
      MetaScreen    *screen;
      MetaWorkspace *workspace;
      MutterPlugin  *plugin = moblin_netbook_get_plugin_singleton ();

      screen    = mutter_plugin_get_screen (plugin);
      workspace = meta_screen_get_active_workspace (screen);

      if (workspace)
        {
          gfloat x, y, w, h;
          gint   xi, yi, wi, hi;
          gint   max_height;

          meta_workspace_get_work_area_all_monitors (workspace, &r);

          clutter_actor_get_position (mcw, &x, &y);
          clutter_actor_get_size (mcw, &w, &h);

          xi  = (gint)x;
          yi  = (gint)y;
          wi  = (gint)w;
          hi  = (gint)h;

          /*
           * Maximum height of the panel is the available working height plus
           * the height of the panel shadow (we allow the shaddow to stretch out
           * of the available area).
           */
          /* FIXME -- devise a way of doing the shadow */
          max_height = r.y + r.height - yi /*+ MNB_PANEL_OOP_SHADOW_HEIGHT*/;

          /*
           * We have to test the size of the child here, as the external size
           * might not be correct (e.g., when initially showing the Switcher the
           * external size matches the size of the child, i.e., the styling is
           * not yet applied.
           *
           * When initially showing the OOP panels, the child is not yet
           * allocated, and will have size 0; in that case we force the resize
           * otherwise the child would get allocated with the size specified at
           * construction.
           *
           * We rest the size whenever the max height does not match that of the
           * child, so that the window gets resized in both up and down (the
           * height passed into mpl_panel_oop_set_height() is really a maximum
           * allowable height, and if the panel client asked for height lesser
           * than this, the original height request will be respected.
           */
          if (max_height != hi || r.width != wi)
            {
              mnb_panel_oop_set_size ((MnbPanel*)panel, r.width, max_height);
            }
        }
    }
}

void
mnb_panel_oop_show_animate (MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;
  MutterPlugin    *plugin = moblin_netbook_get_plugin_singleton ();
  gfloat x, y;
  gfloat height, width;
  ClutterAnimation *animation;
  ClutterActor *toolbar;
  ClutterActor *mcw = (ClutterActor*)priv->mcw;

  if (!mcw)
    {
      g_warning ("Trying to show panel that has no associated window");
      return;
    }

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (panel, "show");
      return;
    }

  if (priv->hide_completed_id)
    {
      g_signal_handler_disconnect (priv->hide_anim, priv->hide_completed_id);
      priv->hide_anim = NULL;
      priv->hide_completed_id = 0;
      priv->in_hide_animation = FALSE;
    }

  mnb_panel_oop_ensure_size (panel);

  /*
   * Check the toolbar is visible, if not show it.
   */
  toolbar = moblin_netbook_get_toolbar (plugin);

  if (!toolbar)
    {
      g_warning ("Cannot show Panel that is not associated with the Toolbar.");
      return;
    }

  if (!CLUTTER_ACTOR_IS_MAPPED (toolbar))
    {
      /*
       * We need to show the toolbar first, and only when it is visible
       * to show this panel.
       */
      g_signal_connect (toolbar, "show-completed",
                        G_CALLBACK (mnb_toolbar_show_completed_cb),
                        panel);

      clutter_actor_show (toolbar);
      return;
    }

  g_signal_emit_by_name (panel, "show-begin");

  clutter_actor_get_position (mcw, &x, &y);
  clutter_actor_get_size (mcw, &width, &height);

  clutter_actor_set_position (mcw, x, -height);

  priv->in_show_animation = TRUE;

  g_object_ref (mcw);

  animation = clutter_actor_animate (mcw, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "x", x,
                                     "y", y,
                                     NULL);

  priv->show_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_panel_oop_show_completed_cb),
                            mcw);
  priv->show_anim = animation;
}

static void
mnb_panel_oop_show (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (panel, "show");
      return;
    }

  if (priv->hide_completed_id)
    {
      g_signal_handler_disconnect (priv->hide_anim, priv->hide_completed_id);
      priv->hide_anim = NULL;
      priv->hide_completed_id = 0;
      priv->in_hide_animation = FALSE;
    }

  org_moblin_UX_Shell_Panel_show_async (priv->proxy,
                                        mnb_panel_oop_dbus_dumb_reply_cb,
                                        NULL);
}

static void
mnb_panel_oop_hide_completed_cb (ClutterAnimation *anim, MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;
  MutterPlugin       *plugin = moblin_netbook_get_plugin_singleton ();

  priv->hide_anim = NULL;
  priv->hide_completed_id = 0;

  if (priv->hide_toolbar)
    {
      /*
       * If the hide_toolbar flag is set, we attempt to hide the Toolbar now
       * that the panel is hidden.
       */
      ClutterActor *toolbar = moblin_netbook_get_toolbar (plugin);

      if (toolbar)
        mnb_toolbar_hide ((MnbToolbar*)toolbar);

      priv->hide_toolbar = FALSE;
    }

  priv->in_hide_animation = FALSE;
  g_signal_emit_by_name (panel, "hide-completed");
  g_object_unref (priv->mcw);
}

void
mnb_panel_oop_hide_animate (MnbPanelOop *panel)
{
  MnbPanelOopPrivate  *priv = panel->priv;
  ClutterAnimation    *animation;
  ClutterActor        *mcw = (ClutterActor*)priv->mcw;

  if (!mcw)
    {
      g_warning ("Attempted to hide a panel with no associated window");
      return;
    }

  if (priv->in_hide_animation)
    {
      g_signal_stop_emission_by_name (panel, "hide");
      return;
    }

  priv->in_hide_animation = TRUE;

  if (priv->show_completed_id)
    {
      g_signal_handler_disconnect (priv->show_anim, priv->show_completed_id);
      priv->show_anim = NULL;
      priv->show_completed_id = 0;
      priv->in_show_animation = FALSE;
      priv->hide_toolbar = FALSE;

      if (priv->button)
        {
          if (nbtk_button_get_checked (priv->button))
            nbtk_button_set_checked (priv->button, FALSE);
        }
    }

  g_signal_emit_by_name (panel, "hide-begin");

  /* de-activate the button */
  if (priv->button)
    {
      /* hide is hooked into the notify::checked signal from the button, so
       * make sure we don't get into a loop by checking checked first
       */
      if (nbtk_button_get_checked (priv->button))
        nbtk_button_set_checked (priv->button, FALSE);
    }

  g_object_ref (mcw);

  animation = clutter_actor_animate (mcw, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "y", -clutter_actor_get_height (mcw),
                                     NULL);

  priv->hide_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_panel_oop_hide_completed_cb),
                            panel);

  priv->hide_anim = animation;
}

static void
mnb_panel_oop_hide (MnbPanel *panel)
{
  MnbPanelOopPrivate  *priv = MNB_PANEL_OOP (panel)->priv;

  if (priv->in_hide_animation)
    {
      g_signal_stop_emission_by_name (panel, "hide");
      return;
    }

  org_moblin_UX_Shell_Panel_hide_async (priv->proxy,
                                        mnb_panel_oop_dbus_dumb_reply_cb,
                                        NULL);
}

static void
mnb_panel_oop_get_size (MnbPanel *panel, guint *width, guint *height)
{
  MnbPanelOopPrivate  *priv = MNB_PANEL_OOP (panel)->priv;
  gfloat               w = 0.0, h = 0.0;

  if (priv->mcw)
    clutter_actor_get_size (CLUTTER_ACTOR (priv->mcw), &w, &h);

  if (width)
    *width = w;

  if (height)
    *height = h;
}

static void
mnb_panel_iface_init (MnbPanelIface *iface)
{
  iface->show             = mnb_panel_oop_show;
  iface->show_begin       = mnb_panel_oop_show_begin;
  iface->show_completed   = mnb_panel_oop_show_completed;
  iface->hide             = mnb_panel_oop_hide;
  iface->hide_begin       = mnb_panel_oop_hide_begin;
  iface->hide_completed   = mnb_panel_oop_hide_completed;

  iface->get_name         = mnb_panel_oop_get_name;
  iface->get_tooltip      = mnb_panel_oop_get_tooltip;
  iface->get_button_style = mnb_panel_oop_get_button_style;
  iface->get_stylesheet   = mnb_panel_oop_get_stylesheet;

  iface->set_size         = mnb_panel_oop_set_size;
  iface->get_size         = mnb_panel_oop_get_size;
}

