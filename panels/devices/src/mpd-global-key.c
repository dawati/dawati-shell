
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

#include <gdk/gdkx.h>
#include "mpd-global-key.h"

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

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_global_key_parent_class)->dispose (object);
}

static void
mpd_global_key_class_init (MpdGlobalKeyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdGlobalKeyPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

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

MpdGlobalKey*
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

