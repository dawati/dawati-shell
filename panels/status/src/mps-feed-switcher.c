/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.:w
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <libsocialweb-client/sw-client.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "mps-feed-switcher.h"
#include "mps-feed-pane.h"

G_DEFINE_TYPE (MpsFeedSwitcher, mps_feed_switcher, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_FEED_SWITCHER, MpsFeedSwitcherPrivate))

typedef struct _MpsFeedSwitcherPrivate MpsFeedSwitcherPrivate;

struct _MpsFeedSwitcherPrivate {
  SwClient *client;

  GHashTable *services;
  GHashTable *service_to_panes;
  GHashTable *service_to_buttons;

  ClutterActor *button_box;
  ClutterActor *placeholder_frame;
  ClutterActor *placeholder_label;
  ClutterActor *notebook;
  ClutterActor *add_new_service_button;

  MxButtonGroup *button_group;
};

enum
{
  PROP_0,
  PROP_CLIENT
};

static void
mps_feed_switcher_ensure_service (MpsFeedSwitcher *switcher,
                                  SwClientService *service);
static void
mps_feed_switcher_remove_service (MpsFeedSwitcher *switcher,
                                  SwClientService *service);

static void
mps_feed_switcher_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CLIENT:
      g_value_set_object (value, priv->client);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_feed_switcher_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CLIENT:
      priv->client = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_feed_switcher_dispose (GObject *object)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  G_OBJECT_CLASS (mps_feed_switcher_parent_class)->dispose (object);
}

static void
mps_feed_switcher_finalize (GObject *object)
{
  G_OBJECT_CLASS (mps_feed_switcher_parent_class)->finalize (object);
}

static gboolean
_has_cap (const gchar **caps,
          const gchar *cap)
{
  if (!caps)
    return FALSE;

  while (*caps)
  {
    if (g_str_equal (*caps, cap))
      return TRUE;

    caps++;
  }

  return FALSE;
}

static void
_service_caps_changed_cb (SwClientService  *service,
                          const gchar     **caps,
                          gpointer          userdata)
{
  MpsFeedSwitcher *switcher = MPS_FEED_SWITCHER (userdata);
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);

  if (_has_cap (caps, IS_CONFIGURED))
  {
    g_debug (G_STRLOC ": Capabilities changed: Service %s has "
             "%s capability",
             sw_client_service_get_name (service),
             IS_CONFIGURED);
    mps_feed_switcher_ensure_service (switcher, service);
  } else {
    g_debug (G_STRLOC ": Capabilities changed: Service doesn't "
             "have %s capability",
             sw_client_service_get_name (service),
             IS_CONFIGURED);
    mps_feed_switcher_remove_service (switcher, service);
  }
}

static void
_service_get_dynamic_caps_cb (SwClientService  *service,
                              const gchar     **caps,
                              const GError     *error,
                              gpointer          userdata)
{
  MpsFeedSwitcher *switcher = MPS_FEED_SWITCHER (userdata);
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);

  if (_has_cap (caps, IS_CONFIGURED))
  {
    g_debug (G_STRLOC ": Service %s has %s dynamic capability",
             sw_client_service_get_name (service),
             IS_CONFIGURED);

    mps_feed_switcher_ensure_service (switcher, service);
  } else {
    g_debug (G_STRLOC ": Service %s doesn't have %s dynamic "
             "capability",
             sw_client_service_get_name (service),
             IS_CONFIGURED);

    mps_feed_switcher_remove_service (switcher, service);
  }
}


static void
_service_get_static_caps_cb (SwClientService  *service,
                             const gchar     **caps,
                             const GError     *error,
                             gpointer          userdata)
{
  MpsFeedSwitcher *switcher = MPS_FEED_SWITCHER (userdata);
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);

  if (!_has_cap (caps, HAS_UPDATE_STATUS_IFACE))
  {
    g_hash_table_remove (priv->services,
                         sw_client_service_get_name (service));
  } else {
    /* Now monitor dynamic caps */
    g_signal_connect (service,
                      "capabilities-changed",
                      (GCallback)_service_caps_changed_cb,
                      userdata);

    g_debug (G_STRLOC ": Service %s has %s static capability",
             sw_client_service_get_name (service),
             HAS_UPDATE_STATUS_IFACE);


    sw_client_service_get_dynamic_capabilities (service,
                                                _service_get_dynamic_caps_cb,
                                                userdata);
  }
}

static void
_client_get_services_cb (SwClient    *client,
                         const GList *services,
                         gpointer     userdata)
{
  MpsFeedSwitcher *switcher = MPS_FEED_SWITCHER (userdata);
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);
  GList *l;

  /* For each service, find out if it has a static cap that indicates we need
   * to think about monitoring it for a dynamic cap.
   */

  for (l = (GList *)services; l; l = l->next)
  {
    SwClientService *service;
    const gchar *service_name = (const gchar *)l->data;

    service = sw_client_get_service (priv->client, service_name);

    g_debug (G_STRLOC ": Got service: %s", service_name);

    g_hash_table_insert (priv->services,
                         g_strdup (service_name),
                         service); /* Own the first ref */

    sw_client_service_get_static_capabilities (service,
                                               _service_get_static_caps_cb,
                                               userdata);
  }
}

static void
mps_feed_switcher_constructed (GObject *object)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (object);

  sw_client_get_services (priv->client,
                          _client_get_services_cb,
                          object);

  if (G_OBJECT_CLASS (mps_feed_switcher_parent_class)->constructed)
    G_OBJECT_CLASS (mps_feed_switcher_parent_class)->constructed (object);
}

static void
mps_feed_switcher_class_init (MpsFeedSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MpsFeedSwitcherPrivate));

  object_class->get_property = mps_feed_switcher_get_property;
  object_class->set_property = mps_feed_switcher_set_property;
  object_class->dispose = mps_feed_switcher_dispose;
  object_class->finalize = mps_feed_switcher_finalize;
  object_class->constructed = mps_feed_switcher_constructed;

  pspec = g_param_spec_object ("client",
                               "Client",
                               "Client",
                               SW_TYPE_CLIENT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CLIENT, pspec);
}

static void
_button_group_active_button_changed_cb (MxButtonGroup *button_group,
                                        GParamSpec    *pspec,
                                        gpointer       userdata)
{
  MpsFeedSwitcher *switcher = MPS_FEED_SWITCHER (userdata);
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);
  GList *children;
  ClutterActor *pane;
  MxButton *button;

  button = mx_button_group_get_active_button (button_group);

  if (button)
  {
    pane = g_object_get_data (G_OBJECT (button), "mps-switcher-pane");

    mx_notebook_set_current_page (MX_NOTEBOOK (priv->notebook),
                                  pane);

  }
}


void meego_status_panel_hide (void);

static void
_new_service_button_clicked_cb (MxButton        *button,
                                MpsFeedSwitcher *switcher)
{
  GDesktopAppInfo *app_info;
  GError *error = NULL;
  const gchar *args[3] = { NULL, };

  app_info = g_desktop_app_info_new ("gnome-control-center.desktop");
  args[0] = g_app_info_get_commandline (G_APP_INFO (app_info));
  args[1] = "bisho.desktop";
  args[2] = NULL;

  if (!g_spawn_async (NULL,
                      (gchar **)args,
                      NULL,
                      G_SPAWN_SEARCH_PATH,
                      NULL,
                      NULL,
                      NULL,
                      &error))
  {
    g_warning (G_STRLOC ": Error starting control center for bisho: %s",
               error->message);
    g_clear_error (&error);
  } else {
    meego_status_panel_hide ();
  }

  g_object_unref (app_info);
}

static void
mps_feed_switcher_init (MpsFeedSwitcher *self)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;

  priv->button_box = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->button_box), 8);

  mx_stylable_set_style_class (MX_STYLABLE (priv->button_box),
                               "mps-switcher-button-box");

  priv->add_new_service_button = mx_button_new_with_label (_("Add new web account"));
  mx_stylable_set_style_class (MX_STYLABLE (priv->add_new_service_button),
                               "mps-switcher-new-service");
  g_signal_connect (priv->add_new_service_button,
                    "clicked",
                    (GCallback)_new_service_button_clicked_cb,
                    self);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               priv->add_new_service_button);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->button_box),
                               priv->add_new_service_button,
                               "expand", TRUE,
                               NULL);

  priv->notebook = mx_notebook_new ();
  clutter_actor_set_reactive (priv->notebook, TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (priv->notebook),
                              "mps-switcher-notebook");

  priv->placeholder_frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->placeholder_frame),
                               "mps-switcher-placeholder-frame");
  priv->placeholder_label = mx_label_new_with_text (_("You don't appear to have any web "
                                                      "services configured or there is "
                                                      "a problem with their configuration. "
                                                      "Use the button above to open the My "
                                                      "Web Accounts and set one up."));
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->placeholder_label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text),
                              TRUE);

  mx_stylable_set_style_class (MX_STYLABLE (priv->placeholder_label),
                               "mps-switcher-placeholder-label");
  mx_bin_set_child (MX_BIN (priv->placeholder_frame),
                    priv->placeholder_label);
  mx_bin_set_alignment (MX_BIN (priv->placeholder_frame),
                        MX_ALIGN_START,
                        MX_ALIGN_START);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->notebook),
                               priv->placeholder_frame);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->button_box,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->notebook,
                                      1, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  priv->services = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          g_object_unref);
  priv->service_to_panes = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  g_object_unref);
  priv->service_to_buttons = g_hash_table_new_full (g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    NULL);

  priv->button_group = mx_button_group_new ();

  g_signal_connect (priv->button_group,
                    "notify::active-button",
                    (GCallback)_button_group_active_button_changed_cb,
                    self);
}

ClutterActor *
mps_feed_switcher_new (SwClient *client)
{
  return g_object_new (MPS_TYPE_FEED_SWITCHER,
                       "client", client,
                       NULL);
}


static void
mps_feed_switcher_ensure_service (MpsFeedSwitcher *switcher,
                                  SwClientService *service)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);
  const gchar *service_name;
  ClutterActor *pane, *button;

  service_name = sw_client_service_get_name (service);

  g_debug (G_STRLOC ": Asked to ensure service %s", service_name);

  pane = g_hash_table_lookup (priv->service_to_panes,
                              service_name);


  if (!pane)
  {
    pane = mps_feed_pane_new (priv->client,
                              service);
    clutter_actor_set_reactive (pane, TRUE);

    g_hash_table_insert (priv->service_to_panes,
                         g_strdup (service_name),
                         g_object_ref_sink (pane));
  }

  if (!clutter_actor_get_parent (pane))
  {
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->notebook), pane);
  }

  button = g_hash_table_lookup (priv->service_to_buttons,
                                service_name);
  if (!button)
  {
    const gchar *display_name;

    display_name = sw_client_service_get_display_name (service);
    button = mx_button_new_with_label (display_name);
    mx_button_set_is_toggle (MX_BUTTON (button), TRUE);
    mx_button_group_add (priv->button_group, MX_BUTTON (button));

    g_hash_table_insert (priv->service_to_buttons,
                         g_strdup (service_name),
                         button);

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                                 button);
    clutter_container_child_set (CLUTTER_CONTAINER (priv->button_box),
                                 button,
                                 "expand", TRUE,
                                 NULL);
    mx_stylable_set_style_class (MX_STYLABLE (button),
                                 "mps-switcher-button");

    clutter_container_raise_child (CLUTTER_CONTAINER (priv->button_box),
                                   priv->add_new_service_button,
                                   NULL);

    g_object_set_data (G_OBJECT (button), "mps-switcher-pane", pane);

  }

  /* Ensure at least one button is active */
  if (!mx_button_group_get_active_button (priv->button_group))
  {
    mx_button_group_set_active_button (priv->button_group,
                                       MX_BUTTON (button));
  }
}


static void
mps_feed_switcher_remove_service (MpsFeedSwitcher *switcher,
                                  SwClientService *service)
{
  MpsFeedSwitcherPrivate *priv = GET_PRIVATE (switcher);
  const gchar *service_name;
  ClutterActor *pane;
  ClutterActor *button;

  service_name = sw_client_service_get_name (service);

  g_debug (G_STRLOC ": Asked to remove %s service",
           service_name);

  button = g_hash_table_lookup (priv->service_to_buttons,
                                service_name);

  if (button)
  {
    mx_button_group_remove (priv->button_group, MX_BUTTON (button));
    mx_button_group_set_active_button (priv->button_group, NULL);
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->button_box),
                                    button);
    g_hash_table_remove (priv->service_to_buttons,
                         service_name);
  }

  pane = g_hash_table_lookup (priv->service_to_panes,
                              service_name);

  if (pane && clutter_actor_get_parent (pane))
  {
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->notebook),
                                    pane);
  }
}
