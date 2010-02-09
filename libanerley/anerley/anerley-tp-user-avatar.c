/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2010, Intel Corporation.
 *
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
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
 *
 */

#include <telepathy-glib/telepathy-glib.h>

#include "anerley-tp-user-avatar.h"

 #define GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANERLEY_TYPE_TP_USER_AVATAR, AnerleyTpUserAvatarPrivate))

G_DEFINE_TYPE (AnerleyTpUserAvatar, anerley_tp_user_avatar, CLUTTER_TYPE_TEXTURE);

typedef struct _AnerleyTpUserAvatarPrivate AnerleyTpUserAvatarPrivate;
struct _AnerleyTpUserAvatarPrivate
{
  TpAccountManager *am;
  GList *account_ptr;
  TpProxySignalConnection *avatar_changed_signal;
};

static void _get_next_avatar (AnerleyTpUserAvatar *self);
static void _get_avatar_cb (GObject *account,
                            GAsyncResult *res,
                            gpointer user_data);

static void
anerley_tp_user_avatar_get_first_avatar (AnerleyTpUserAvatar *self)
{
  AnerleyTpUserAvatarPrivate *priv = GET_PRIVATE (self);

  priv->account_ptr = tp_account_manager_get_valid_accounts (priv->am);
  _get_next_avatar (self);
}

static void
_avatar_changed_cb (TpAccount *account,
                    gpointer user_data,
                    GObject *self)
{
  /* rerequest the avatar */
  tp_account_get_avatar_async (account, _get_avatar_cb, self);
}

static void
_get_avatar_cb (GObject *account,
                GAsyncResult *res,
                gpointer user_data)
{
  AnerleyTpUserAvatar *self = ANERLEY_TP_USER_AVATAR (user_data);
  AnerleyTpUserAvatarPrivate *priv = GET_PRIVATE (self);
  const GArray *data;
  char *path;
  GError *error = NULL;

  data = tp_account_get_avatar_finish (TP_ACCOUNT (account), res, &error);
  if (error != NULL)
    {
      g_warning (G_STRLOC ": failed to get avatar for %s: %s",
                 tp_proxy_get_object_path (account), error->message);

      g_clear_error (&error);
      _get_next_avatar (self);

      return;
    }

  if (data->len == 0)
    {
      g_debug ("No avatar for %s",
               tp_proxy_get_object_path (priv->account_ptr->data));

      if (priv->account_ptr != NULL)
        _get_next_avatar (self);
      else
        anerley_tp_user_avatar_get_first_avatar (self);

      return;
    }

  g_debug ("Account %s has %u byte avatar",
      tp_proxy_get_object_path (account), data->len);

  /* free the account list */
  priv->account_ptr = g_list_first (priv->account_ptr);
  g_list_free (priv->account_ptr);
  priv->account_ptr = NULL;

  /* it would be secretly brilliant here if ClutterTexture accepted an inline
   * data stream */
  path = g_build_filename (g_get_user_cache_dir (),
                           "anerley",
                           "avatars",
                           "user-avatar",
                           NULL);

  if (!g_file_set_contents (path, data->data, data->len, &error))
    {
      g_warning (G_STRLOC ": Unable to set file contents: %s",
          error->message);

      goto out;
    }

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (self), path, &error))
    {
      g_warning (G_STRLOC ": Unable to set texture contents: %s",
          error->message);

      goto out;
    }

  /* watch for avatar changes */
  if (priv->avatar_changed_signal != NULL)
    tp_proxy_signal_connection_disconnect (priv->avatar_changed_signal);

  priv->avatar_changed_signal =
    tp_cli_account_interface_avatar_connect_to_avatar_changed (
        TP_ACCOUNT (account), _avatar_changed_cb,
        NULL, NULL, G_OBJECT (self), &error);
  if (error != NULL)
    {
      g_warning (G_STRLOC ": Unable to monitor for avatar changes: %s",
          error->message);

      goto out;
    }

out:
  g_free (path);
  g_clear_error (&error);
}

static void
_get_next_avatar (AnerleyTpUserAvatar *self)
{
  AnerleyTpUserAvatarPrivate *priv = GET_PRIVATE (self);

  tp_account_get_avatar_async (TP_ACCOUNT (priv->account_ptr->data),
      _get_avatar_cb, self);

  if (priv->account_ptr->next != NULL)
    {
      priv->account_ptr = priv->account_ptr->next;
    }
  else
    {
      g_debug ("no more accounts to try");

      priv->account_ptr = g_list_first (priv->account_ptr);
      g_list_free (priv->account_ptr);
      priv->account_ptr = NULL;
    }
}

static void
_account_manager_ready (GObject *am,
                        GAsyncResult *res,
                        gpointer user_data)
{
  AnerleyTpUserAvatar *self = ANERLEY_TP_USER_AVATAR (user_data);
  GError *error = NULL;

  if (!tp_account_manager_prepare_finish (TP_ACCOUNT_MANAGER (am), res, &error))
    {
      g_critical (G_STRLOC ": failed to set up account: %s", error->message);
      g_clear_error (&error);
      return;
    }

  g_debug ("Account Manager ready\n");

  anerley_tp_user_avatar_get_first_avatar (self);
}

static void
anerley_tp_user_avatar_class_init (AnerleyTpUserAvatarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (gobject_class, sizeof (AnerleyTpUserAvatarPrivate));
}

static void
anerley_tp_user_avatar_init (AnerleyTpUserAvatar *self)
{
  AnerleyTpUserAvatarPrivate *priv = GET_PRIVATE (self);

  priv->am = tp_account_manager_dup ();

  tp_account_manager_prepare_async (priv->am, NULL,
      _account_manager_ready, self);
}

ClutterActor *
anerley_tp_user_avatar_new (void)
{
  return g_object_new (ANERLEY_TYPE_TP_USER_AVATAR, NULL);
}
