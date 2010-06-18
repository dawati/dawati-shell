/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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


#include <gconf/gconf-client.h>

#include "penge-view-background.h"

#define KEY_DIR "/desktop/meego/myzone"
#define KEY_BG_FILENAME KEY_DIR "/background_filename"

G_DEFINE_TYPE (PengeViewBackground, penge_view_background, PENGE_TYPE_MAGIC_TEXTURE)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_VIEW_BACKGROUND, PengeViewBackgroundPrivate))

#define GET_PRIVATE(o) ((PengeViewBackground *)o)->priv

struct _PengeViewBackgroundPrivate {
  GConfClient *client;
  guint key_notify_id;
};

static void
penge_view_background_dispose (GObject *object)
{
  PengeViewBackgroundPrivate *priv = GET_PRIVATE (object);
  GError *error = NULL;
  
  if (priv->client)
  {
    gconf_client_notify_remove (priv->client,
                                priv->key_notify_id);

    gconf_client_remove_dir (priv->client,
                             KEY_DIR,
                             &error);

    if (error)
    {
      g_warning (G_STRLOC ": Error when removing notification directory: %s",
                 error->message);
      g_clear_error (&error);
    }

    g_object_unref (priv->client);
    priv->client = NULL;
  }

  G_OBJECT_CLASS (penge_view_background_parent_class)->dispose (object);
}

static void
penge_view_background_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_view_background_parent_class)->finalize (object);
}

static void
penge_view_background_class_init (PengeViewBackgroundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeViewBackgroundPrivate));

  object_class->dispose = penge_view_background_dispose;
  object_class->finalize = penge_view_background_finalize;
}

static void
bg_filename_changed_cb (GConfClient *client,
                        guint        cnxn_id,
                        GConfEntry  *entry,
                        gpointer     userdata)
{
  PengeViewBackground *pvb = PENGE_VIEW_BACKGROUND (userdata);
  const gchar *filename;
  GConfValue *value;
  GError *error = NULL;

  value = gconf_entry_get_value (entry);

  if (value)
  {
    filename = gconf_value_get_string (value);
    if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (pvb),
                                        filename,
                                        &error))
    {
      g_warning (G_STRLOC ": Error setting magic texture contents: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      clutter_actor_set_opacity ((ClutterActor *)pvb, 0xff);
    }
  } else {
    /* If the key is unset let's just make ourselves invisible */
    clutter_actor_set_opacity ((ClutterActor *)pvb, 0x0);
  }
}

static void
penge_view_background_init (PengeViewBackground *self)
{
  PengeViewBackgroundPrivate *priv = GET_PRIVATE_REAL (self);
  GError *error = NULL;

  self->priv = priv;

  priv->client = gconf_client_get_default ();
  gconf_client_add_dir (priv->client,
                        KEY_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error when adding directory for notification: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->key_notify_id = gconf_client_notify_add (priv->client,
                                                 KEY_BG_FILENAME,
                                                 bg_filename_changed_cb,
                                                 self,
                                                 NULL,
                                                 &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error when adding key for notification: %s",
               error->message);
    g_clear_error (&error);
  }

  /* Trick us into reading the background. Aren't we sneaky muahaha! */
  gconf_client_notify (priv->client, KEY_BG_FILENAME);
}

