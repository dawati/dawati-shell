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
static void mnb_panel_oop_button_toggled_cb (NbtkWidget *button,
                                             GParamSpec *pspec,
                                             MnbPanel   *panel);
static void mnb_panel_oop_show_animate      (MnbPanelOop *panel);

enum
{
  PROP_0,

  PROP_DBUS_NAME,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
};

enum
{
  READY,
  REMOTE_PROCESS_DIED,
  DESTROY,

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
  gchar           *child_class;

  gint             x;
  gint             y;
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
  gboolean         mapped            : 1;
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
    case PROP_X:
      g_value_set_int (value, priv->x);
      break;
    case PROP_Y:
      g_value_set_int (value, priv->y);
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
    case PROP_X:
      priv->x = g_value_get_int (value);
      break;
    case PROP_Y:
      priv->y = g_value_get_int (value);
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

  if (priv->mcw)
    moblin_netbook_activate_mutter_window (priv->mcw);
}

/*
 * Callbacks for the request signals exposed by the panels.
 */
static void
mnb_panel_oop_request_focus_cb (DBusGProxy *proxy, MnbPanelOop *panel)
{
  if (!mnb_panel_is_mapped ((MnbPanel*)panel))
    {
      g_warning ("Panel %s requested focus while not visible !!!",
                 mnb_panel_oop_get_name ((MnbPanel*)panel));
      return;
    }

  mnb_panel_oop_focus (panel);
}

static void
mnb_panel_oop_request_button_style_cb (DBusGProxy  *proxy,
                                       const gchar *style_id,
                                       MnbPanelOop    *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  g_free (priv->button_style_id);
  priv->button_style_id = g_strdup (style_id);

  g_signal_emit_by_name (panel, "request-button-style", style_id);
}

static void
mnb_panel_oop_request_tooltip_cb (DBusGProxy  *proxy,
                                  const gchar *tooltip,
                                  MnbPanelOop    *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  g_free (priv->tooltip);
  priv->tooltip = g_strdup (tooltip);

  g_signal_emit_by_name (panel, "request-tooltip", tooltip);
}

static void
mnb_panel_oop_set_size_cb (DBusGProxy  *proxy,
                           guint        width,
                           guint        height,
                           MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  priv->width  = width;
  priv->height = height;
}

static void
mnb_panel_oop_set_position_cb (DBusGProxy  *proxy,
                               gint         x,
                               gint         y,
                               MnbPanelOop *panel)
{
  MnbPanelOopPrivate *priv = panel->priv;

  priv->x = x;
  priv->y = y;
}

static void
mnb_panel_oop_dispose (GObject *self)
{
  MnbPanelOopPrivate *priv   = MNB_PANEL_OOP (self)->priv;
  DBusGProxy         *proxy  = priv->proxy;

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

  if (priv->button)
    {
      g_signal_handlers_disconnect_by_func (priv->button,
                                            mnb_panel_oop_button_toggled_cb,
                                            self);
      priv->button = NULL;
    }

  g_signal_emit (self, signals[DESTROY], 0);

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
                                   PROP_X,
                                   g_param_spec_int ("x",
                                                     "X coordinate",
                                                     "X coordiante",
                                                      0, G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y coordinate",
                                                     "Y coordiante",
                                                      0, G_MAXINT,
                                                      0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

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

  signals[DESTROY] =
    g_signal_new ("destroy",
		  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_CLEANUP |
                  G_SIGNAL_NO_RECURSE  |
                  G_SIGNAL_NO_HOOKS,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  dbus_g_object_register_marshaller (moblin_netbook_marshal_VOID__UINT_UINT,
                                     G_TYPE_NONE,
                                     G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
  dbus_g_object_register_marshaller (moblin_netbook_marshal_VOID__INT_INT,
                                     G_TYPE_NONE,
                                     G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
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

  mnb_panel_oop_focus (MNB_PANEL_OOP (self));

  org_moblin_UX_Shell_Panel_show_end_async (priv->proxy,
                                            mnb_panel_oop_dbus_dumb_reply_cb,
                                            NULL);
}

static void
mnb_panel_oop_hide_begin (MnbPanel *self)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (self)->priv;

  priv->hide_in_progress = TRUE;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

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

  if (error)
    {
      g_warning ("Could not initialize Panel %s: %s",
                 mnb_panel_oop_get_name ((MnbPanel*)panel), error->message);
      g_object_unref (panel);
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

  priv->xid = xid;

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

            XFree (r_prop);
          }
      }

    meta_error_trap_pop (display, TRUE);
  }

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
                                          priv->x,
                                          priv->y,
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

  dbus_g_proxy_add_signal (proxy, "SetPosition",
                           G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "SetPosition",
                               G_CALLBACK (mnb_panel_oop_set_position_cb),
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
mnb_panel_oop_new (const gchar  *dbus_name,
                   gint          x,
                   gint          y,
                   guint         width,
                   guint         height)
{
  MnbPanelOop *panel = g_object_new (MNB_TYPE_PANEL_OOP,
                                     "dbus-name",     dbus_name,
                                     "x",             x,
                                     "y",             y,
                                     "width",         width,
                                     "height",        height,
                                     NULL);

  if (!panel->priv->constructed)
    {
      g_warning (G_STRLOC " Construction of Panel for %s failed.", dbus_name);

      g_object_unref (panel);
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
}

void
mnb_panel_oop_show_mutter_window (MnbPanelOop *panel, MutterWindow *mcw)
{
  MnbPanelOopPrivate *priv;

  g_return_if_fail (MNB_IS_PANEL_OOP (panel));

  priv = panel->priv;

  if (!mcw)
    {
      if (priv->mcw)
        {
          g_signal_handlers_disconnect_by_func (priv->mcw,
                                        mnb_panel_oop_mutter_window_destroy_cb,
                                        panel);

          priv->mcw = NULL;
        }

      return;
    }

  if (mcw == priv->mcw)
    return;
  else if (priv->mcw)
    {
      g_signal_handlers_disconnect_by_func (priv->mcw,
                                        mnb_panel_oop_mutter_window_destroy_cb,
                                        panel);
    }

  priv->mcw = mcw;

  if (!mcw)
    {
      g_warning (G_STRLOC " Asked to show panel with no MutterWindow !!!");
      return;
    }

  g_signal_connect (mcw, "destroy",
                    G_CALLBACK (mnb_panel_oop_mutter_window_destroy_cb),
                    panel);

  priv->mapped = TRUE;

  mnb_panel_oop_show_animate (panel);
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

static void
mnb_panel_oop_set_size (MnbPanel *panel, guint width, guint height)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;
  gfloat w, h;
  gboolean h_change = FALSE, w_change = FALSE;

  if (priv->mcw)
    {
      /*
       * Exit if no change
       */
      clutter_actor_get_size (CLUTTER_ACTOR (priv->mcw), &w, &h);

      if ((guint)w != width)
        w_change = TRUE;

      if ((guint)h != height)
        h_change = TRUE;
    }
  else
    {
      /*
       * If we have no mutter window yet, just force the dbus call.
       */
      w_change = TRUE;
    }

  if (!w_change && !h_change)
    return;

  org_moblin_UX_Shell_Panel_set_size_async (priv->proxy, width, height,
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
}

static void
mnb_toolbar_show_completed_cb (MnbToolbar *toolbar, gpointer data)
{
  MnbPanelOop *panel = data;

  g_signal_handlers_disconnect_by_func (toolbar,
                                        mnb_toolbar_show_completed_cb,
                                        data);

  mnb_panel_oop_show_animate (panel);
}

static void
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
    return;

  if (priv->hide_completed_id)
    {
      g_signal_handler_disconnect (priv->hide_anim, priv->hide_completed_id);
      priv->hide_anim = NULL;
      priv->hide_completed_id = 0;
      priv->in_hide_animation = FALSE;
    }

  mnb_panel_ensure_size ((MnbPanel*)panel);

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

  animation = clutter_actor_animate (mcw, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "x", x,
                                     "y", y,
                                     NULL);

  priv->show_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_panel_oop_show_completed_cb),
                            panel);
  priv->show_anim = animation;
}

static void
mnb_panel_oop_show (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;

  if (priv->in_show_animation)
    return;

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

  mutter_plugin_effect_completed (plugin, priv->mcw, MUTTER_PLUGIN_DESTROY);
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
    return;

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
    return;

  priv->mapped = FALSE;

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
mnb_panel_oop_button_weak_unref_cb (MnbPanelOop *panel, GObject *button)
{
  panel->priv->button = NULL;
}

/*
 * TODO -- there is obviously a degree of overlap between this and the
 * MnbDropDown code; see if we could fold it somewhere to avoid duplication.
 */
static void
mnb_panel_oop_button_toggled_cb (NbtkWidget *button,
                                 GParamSpec *pspec,
                                 MnbPanel   *panel)
{
  if (nbtk_button_get_checked (NBTK_BUTTON (button)))
    {
#if 0
      /*
       * Must reset the y in case a previous animation ended prematurely
       * and the y is not set correctly; see bug 900.
       */
      clutter_actor_set_y (actor, TOOLBAR_HEIGHT);
#endif

      mnb_panel_show (panel);
    }
  else
    mnb_panel_hide (panel);
}

static void
mnb_panel_oop_set_button (MnbPanel *panel, NbtkButton *button)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;
  NbtkButton         *old_button;

  g_return_if_fail (!button || NBTK_IS_BUTTON (button));

  old_button = priv->button;
  priv->button = button;

  if (old_button)
    {
      g_object_weak_unref (G_OBJECT (old_button),
                           (GWeakNotify) mnb_panel_oop_button_weak_unref_cb,
                           panel);

      g_signal_handlers_disconnect_by_func (old_button,
                                 G_CALLBACK (mnb_panel_oop_button_toggled_cb),
                                 panel);
    }

  if (button)
    {
      g_object_weak_ref (G_OBJECT (button),
                         (GWeakNotify) mnb_panel_oop_button_weak_unref_cb,
                         panel);

      g_signal_connect (button,
                        "notify::checked",
                        G_CALLBACK (mnb_panel_oop_button_toggled_cb),
                        panel);
    }
}

static gboolean
mnb_panel_oop_is_mapped (MnbPanel *panel)
{
  MnbPanelOopPrivate *priv;

  g_return_val_if_fail (MNB_IS_PANEL_OOP (panel), FALSE);

  priv = MNB_PANEL_OOP (panel)->priv;

  return priv->mapped;
}

static void
mnb_panel_oop_set_position (MnbPanel *panel, gint x, gint y)
{
  MnbPanelOopPrivate *priv = MNB_PANEL_OOP (panel)->priv;
  gfloat xf, yf;
  gboolean x_change = FALSE, y_change = FALSE;

  if (priv->mcw)
    {
      /*
       * Exit if no change
       */
      clutter_actor_get_position (CLUTTER_ACTOR (priv->mcw), &xf, &yf);

      if ((guint)xf != x)
        x_change = TRUE;

      if ((guint)yf != y)
        y_change = TRUE;
    }
  else
    {
      /*
       * This is the case when the position of the panel is set while it is
       * not mapped; force the dbus call.
       */
      x_change = TRUE;
    }

  if (!x_change && !y_change)
    return;

  org_moblin_UX_Shell_Panel_set_position_async (priv->proxy, x, y,
                                            mnb_panel_oop_dbus_dumb_reply_cb,
                                            NULL);
}

static void
mnb_panel_oop_get_position (MnbPanel *panel, gint *x, gint *y)
{
  MnbPanelOopPrivate  *priv = MNB_PANEL_OOP (panel)->priv;
  gfloat               xf = 0.0, yf = 0.0;

  if (priv->mcw)
    clutter_actor_get_position (CLUTTER_ACTOR (priv->mcw), &xf, &yf);

  if (x)
    *x = xf;

  if (y)
    *y = yf;
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
  iface->set_position     = mnb_panel_oop_set_position;
  iface->get_position     = mnb_panel_oop_get_position;

  iface->set_button       = mnb_panel_oop_set_button;

  iface->is_mapped        = mnb_panel_oop_is_mapped;
}

