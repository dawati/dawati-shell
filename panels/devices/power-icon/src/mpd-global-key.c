
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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
 */

#include <stdbool.h>
#include <gdk/gdkx.h>
#include "mpd-global-key.h"
#include "config.h"

enum
{
  MPD_GLOBAL_KEY_ERROR_BAD_ACCESS_GRABBING_KEY
};

#define MPD_GLOBAL_KEY_ERROR (mpd_global_key_error_quark ())

static GQuark
mpd_global_key_error_quark (void)
{
  static GQuark _quark = 0;
  if (!_quark)
    _quark = g_quark_from_static_string ("mpd-global-key-error");
  return _quark;
}

G_DEFINE_TYPE (MpdGlobalKey, mpd_global_key, MX_TYPE_ACTION)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_GLOBAL_KEY, MpdGlobalKeyPrivate))

enum
{
  PROP_0,

  PROP_KEY_CODE
};

typedef struct
{
  guint key_code;
} MpdGlobalKeyPrivate;

static bool
window_grab_key (GdkWindow   *window,
                 guint        key_code,
                 GError     **error)
{
  Display *dpy = GDK_DISPLAY ();
  guint    mask = AnyModifier;
  int      ret;

  gdk_error_trap_push ();

  ret = XGrabKey (dpy, key_code, mask, GDK_WINDOW_XID (window), True,
                  GrabModeAsync, GrabModeAsync);
  if (BadAccess == ret)
  {
    if (error)
      *error = g_error_new (MPD_GLOBAL_KEY_ERROR,
                            MPD_GLOBAL_KEY_ERROR_BAD_ACCESS_GRABBING_KEY,
                            "%s: 'BadAccess' grabbing key %d",
                            G_STRLOC, key_code);
    gdk_flush ();
    gdk_error_trap_pop ();
    return false;
  }

  /* grab the lock key if possible */
  ret = XGrabKey (dpy, key_code, LockMask | mask, GDK_WINDOW_XID (window), True,
                  GrabModeAsync, GrabModeAsync);
  if (BadAccess == ret)
  {
    if (error)
      *error = g_error_new (MPD_GLOBAL_KEY_ERROR,
                            MPD_GLOBAL_KEY_ERROR_BAD_ACCESS_GRABBING_KEY,
                            "%s: 'BadAccess' grabbing key %d with LockMask",
                            G_STRLOC, key_code);
    gdk_flush ();
    gdk_error_trap_pop ();
    return false;
  }

  gdk_flush ();
  gdk_error_trap_pop ();
  return true;
}

static GdkFilterReturn
_event_filter_cb (XEvent        *xev,
                  GdkEvent      *ev,
                  MpdGlobalKey  *self)
{
  MpdGlobalKeyPrivate *priv = GET_PRIVATE (self);

  if (xev->type == KeyPress &&
      xev->xkey.keycode == priv->key_code)
  {
    g_signal_emit_by_name (self, "activated");
    return GDK_FILTER_REMOVE;
  }

  return GDK_FILTER_CONTINUE;
}

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id) {
  case PROP_KEY_CODE:
    g_value_set_uint (value,
                      mpd_global_key_get_key_code (MPD_GLOBAL_KEY (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               guint         property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  MpdGlobalKeyPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
  case PROP_KEY_CODE:
    /* Construct-only, no notifications and stuff. */
    priv->key_code = g_value_get_uint (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static GObject *
_constructor (GType                  type,
              guint                  n_properties,
              GObjectConstructParam *properties)
{
  MpdGlobalKey *self = (MpdGlobalKey *)
                          G_OBJECT_CLASS (mpd_global_key_parent_class)
                            ->constructor (type, n_properties, properties);
  MpdGlobalKeyPrivate *priv = GET_PRIVATE (self);
  GdkWindow *root_window = gdk_screen_get_root_window (
                              gdk_screen_get_default ());
  GError    *error = NULL;

  g_return_val_if_fail (priv->key_code, NULL);
  g_return_val_if_fail (root_window, NULL);

  window_grab_key (root_window, priv->key_code, &error);
  if (error)
  {
    g_critical ("%s", error->message);
    g_clear_error (&error);
    return NULL;
  }

  gdk_window_add_filter (root_window,
                         (GdkFilterFunc) _event_filter_cb,
                         self);

  return (GObject *) self;
}

static void
_dispose (GObject *object)
{
  MpdGlobalKeyPrivate *priv = GET_PRIVATE (object);

  if (priv->key_code)
  {
    Display *dpy = GDK_DISPLAY ();
    GdkWindow *root_window = gdk_screen_get_root_window (
                                gdk_screen_get_default ());

    gdk_window_remove_filter (root_window,
                              (GdkFilterFunc) _event_filter_cb,
                              object);

    XUngrabKey (dpy, priv->key_code, AnyModifier,
                GDK_WINDOW_XID (root_window));
    XUngrabKey (dpy, priv->key_code, AnyModifier | LockMask,
                GDK_WINDOW_XID (root_window));

    priv->key_code = 0;
  }

  G_OBJECT_CLASS (mpd_global_key_parent_class)->dispose (object);
}

static void
mpd_global_key_class_init (MpdGlobalKeyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdGlobalKeyPrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_KEY_CODE,
                                   g_param_spec_uint ("key-code",
                                                      "Key code",
                                                      "XF86 Key code",
                                                      0, G_MAXUINT, 0,
                                                      param_flags | G_PARAM_CONSTRUCT_ONLY));
}

static void
mpd_global_key_init (MpdGlobalKey *self)
{
}

MxAction *
mpd_global_key_new (guint key_code)
{
  return g_object_new (MPD_TYPE_GLOBAL_KEY,
                       "key-code", key_code,
                       NULL);
}

guint
mpd_global_key_get_key_code (MpdGlobalKey const *self)
{
  MpdGlobalKeyPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_GLOBAL_KEY (self), 0);

  return priv->key_code;
}

