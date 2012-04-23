/*
 * ntf-gio - Monitor volumes and create notifications
 *
 * Copyright © 2012 Intel Corporation.
 *
 * Authors: Michael Wood <michael.g.wood@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ntf-gio.h"
#include "ntf-overlay.h"
#include "ntf-notification.h"
#include "ntf-tray.h"
#include "dawati-netbook-notify-store.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

G_DEFINE_TYPE (NtfGIO, ntf_gio, G_TYPE_OBJECT)

#define NTF_GIO_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NTF_TYPE_GIO, NtfGIOPrivate))

#define GET_PRIVATE(o) NTF_GIO(o)->priv

#define DEFAULT_TIMEOUT 7000

struct _NtfGIOPrivate
{
  GVolumeMonitor *volume_monitor;
};

static void _volume_monitor_mount_added_cb (GVolumeMonitor *volume_monitor,
                                            GMount         *mount,
                                            NtfGIO         *source);

static void
ntf_gio_dispose (GObject *object)
{
  NtfGIOPrivate *priv = GET_PRIVATE (object);

  if (priv->volume_monitor)
    {
      g_signal_handlers_disconnect_by_func (priv->volume_monitor,
                                            _volume_monitor_mount_added_cb,
                                            object);
      g_object_unref (priv->volume_monitor);
      priv->volume_monitor = NULL;
    }

  G_OBJECT_CLASS (ntf_gio_parent_class)->dispose (object);
}

static void
ntf_gio_class_init (NtfGIOClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (NtfGIOPrivate));

  object_class->dispose = ntf_gio_dispose;
}

static const gchar *
_icon_from_mount (GMount *mount)
{
  GVolume *volume;
  GIcon *icon = NULL;
  const gchar *icon_name = NULL;

  volume = g_mount_get_volume (mount);

  if (volume)
    icon = g_volume_get_icon (volume);

  if (!icon)
    icon = g_mount_get_icon (mount);

  if (icon)
    {
      if (G_IS_THEMED_ICON (icon))
        {
          const gchar * const *names;

          names = g_themed_icon_get_names (G_THEMED_ICON (icon));

          icon_name = names[0];
        }
    }

  if (icon)
    g_object_unref (icon);

  return icon_name;
}


static void
_open_clicked_cb (MxButton *button, gpointer user_data)
{
  GError *error = NULL;
  gchar *uri;
  /* TODO get a proper launch context so that we get the startup
   * notification
   */

  uri = (gchar *)g_object_get_data (G_OBJECT (user_data), "uri");

  g_app_info_launch_default_for_uri (uri, NULL, &error);

  g_free (uri);

  if (error)
    {
      g_warning ("Could not spawn supplied action: %s",
                 error->message);

      g_clear_error (&error);
    }

  ntf_notification_close (NTF_NOTIFICATION (user_data));
}

static void
_volume_monitor_mount_added_cb (GVolumeMonitor *volume_monitor,
                                GMount         *mount,
                                NtfGIO         *source)
{
  NtfNotification *notification;
  gchar *name = NULL, *body, *uri;
  GVolume *volume;
  const gchar *icon_name;
  GFile *flocation;
  ClutterActor *button;
  ClutterTexture *icon;

  button = mx_button_new_with_label (_("Open"));
  mx_stylable_set_style_class (MX_STYLABLE (button), "Primary");

  volume = g_mount_get_volume (mount);
  flocation = g_mount_get_default_location (mount);

  uri = g_file_get_uri (flocation);

  if (volume)
    name = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_LABEL);

  if (!name)
    name = g_mount_get_name (mount);

  icon_name = _icon_from_mount (mount);

  icon = mx_icon_theme_lookup_texture (mx_icon_theme_get_default (),
                                       icon_name,
                                       32);

  body = g_strdup_printf (_("%s has been plugged in. You can use "
                            "the File browser to interact with it"), name);

  /* Create the notification */
  notification = ntf_notification_new (NULL, 0, 9999, FALSE);
  ntf_notification_set_body (notification, body);
  ntf_notification_set_summary (notification, _("USB plugged in"));
  ntf_notification_set_timeout (notification, DEFAULT_TIMEOUT);
  ntf_notification_add_button (notification, button, NULL, 0);
  ntf_notification_set_icon (notification, CLUTTER_ACTOR (icon));

  ntf_tray_add_notification (ntf_overlay_get_tray (FALSE), notification);

  g_object_set_data (G_OBJECT (notification), "uri", uri);

  g_signal_connect (button,
                    "clicked",
                    G_CALLBACK (_open_clicked_cb),
                    notification);

  g_free (name);
  g_free (body);

  if (volume)
    g_object_unref (volume);
}

static void
ntf_gio_init (NtfGIO *self)
{
  self->priv = NTF_GIO_PRIVATE (self);

  self->priv->volume_monitor = g_volume_monitor_get ();

  g_signal_connect (self->priv->volume_monitor,
                    "mount-added",
                    (GCallback)_volume_monitor_mount_added_cb,
                    self);
}

NtfGIO *
ntf_gio_new (void)
{
  return g_object_new (NTF_TYPE_GIO, NULL);
}
