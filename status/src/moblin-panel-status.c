/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib/gi18n.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

#include <mojito-client/mojito-client.h>

#include <libmissioncontrol/mission-control.h>

#include "mnb-web-status-row.h"
#include "mnb-im-status-row.h"

#define ICON_SIZE       48
#define PADDING         8
#define BORDER_WIDTH    4

typedef struct _ServiceInfo
{
  gchar *name;

  MojitoClientService *service;
  ClutterActor *row;
  NbtkTable *table;

  guint can_update : 1;
  guint has_icon   : 1;
  guint is_visible : 1;
} ServiceInfo;

typedef struct _AccountInfo
{
  gchar *name;

  McAccount *account;
  ClutterActor *row;
  NbtkTable *table;

  guint is_visible : 1;
} AccountInfo;

static ClutterActor *empty_bin = NULL;
static guint n_visible = 0;

static void
on_caps_changed (MojitoClientService *service,
                 guint32              new_caps,
                 ServiceInfo         *s_info)
{
  gboolean was_visible, has_row;

  was_visible = s_info->is_visible;
  has_row     = s_info->row != NULL;

  s_info->can_update = (new_caps & MOJITO_CLIENT_SERVICE_CAN_UPDATE_STATUS)    ? TRUE : FALSE;
  s_info->has_icon   = (new_caps & MOJITO_CLIENT_SERVICE_CAN_GET_PERSONA_ICON) ? TRUE : FALSE;

  g_debug ("%s: CapabilitiesChanged['%s']: can-update:%s, has-icon:%s",
           G_STRLOC,
           s_info->name,
           s_info->can_update ? "yes" : "no",
           s_info->has_icon ? "yes" : "no");

  if (s_info->can_update && s_info->has_icon)
    {
      /* the caps we care about haven't changed */
      if (has_row && was_visible)
        return;

      if (!has_row)
        {
          s_info->row = g_object_new (MNB_TYPE_WEB_STATUS_ROW,
                                      "service-name", s_info->name,
                                      NULL);
          nbtk_table_add_actor_with_properties (s_info->table,
                                                s_info->row,
                                                -1, 0,
                                                "row-span", 1,
                                                "col-span", 1,
                                                "x-expand", TRUE,
                                                "y-expand", FALSE,
                                                "x-fill", TRUE,
                                                "y-fill", FALSE,
                                                "x-align", 0.0,
                                                "y-align", 0.0,
                                                "allocate-hidden", FALSE,
                                                NULL);
        }

      if (!was_visible)
        {
          clutter_actor_show (s_info->row);
          s_info->is_visible = TRUE;

          n_visible += 1;
        }

      g_debug ("%s: showing row for service '%s'", G_STRLOC, s_info->name);
    }
  else
    {
      if (!has_row)
        return;

      if (was_visible)
        {
          clutter_actor_hide (s_info->row);
          s_info->is_visible = FALSE;

          n_visible -= 1;
        }

      g_debug ("%s: hiding row for service '%s'", G_STRLOC, s_info->name);
    }

  if (n_visible == 0)
    clutter_actor_show (empty_bin);
  else
    clutter_actor_hide (empty_bin);
}

static void
get_caps (MojitoClientService *service,
          guint32              new_caps,
          const GError        *error,
          gpointer             user_data)
{
  ServiceInfo *s_info = user_data;

  if (error)
    {
      g_critical ("Unable to retrieve capabilities for service '%s': %s",
                  s_info->name,
                  error->message);
      return;
    }

  on_caps_changed (service, new_caps, s_info);
}

static void
on_mojito_get_services (MojitoClient *client,
                        const GList  *services,
                        gpointer      data)
{
  NbtkTable *table = data;
  const GList *s;

  for (s = services; s != NULL; s = s->next)
    {
      const gchar *service_name = s->data;
      MojitoClientService *service;
      ServiceInfo *s_info;

      if (service_name == NULL || *service_name == '\0')
        break;

      /* filter out the dummy service */
      if (strcmp (service_name, "dummy") == 0)
        continue;

      service = mojito_client_get_service (client, service_name);
      if (G_UNLIKELY (service == NULL))
        continue;

      g_debug ("%s: GetServices ['%s']", G_STRLOC, service_name);

      s_info = g_slice_new (ServiceInfo);
      s_info->name = g_strdup (service_name);
      s_info->service = service;
      s_info->row = NULL;
      s_info->table = table;
      s_info->can_update = FALSE;
      s_info->has_icon = FALSE;
      s_info->is_visible = FALSE;

      g_signal_connect (s_info->service, "capabilities-changed",
                        G_CALLBACK (on_caps_changed),
                        s_info);
      mojito_client_service_get_capabilities (s_info->service,
                                              get_caps,
                                              s_info);
    }
}

static void
update_header (NbtkLabel *header,
               gboolean   is_online)
{
  if (is_online)
    nbtk_label_set_text (header, _("Your current status"));
  else
    nbtk_label_set_text (header, _("Your current status - you are offline"));
}

static void
on_mojito_online_changed (MojitoClient *client,
                          gboolean      is_online,
                          NbtkLabel    *label)
{
  update_header (label, is_online);
}

static void
on_mojito_is_online (MojitoClient *client,
                     gboolean      is_online,
                     gpointer      data)
{
  NbtkLabel *label = data;

  update_header (label, is_online);
}

static gboolean
add_account (gpointer user_data)
{
  AccountInfo *a_info = user_data;

  a_info->row = g_object_new (MNB_TYPE_IM_STATUS_ROW,
                              "account-name", a_info->name,
                              NULL);

  nbtk_table_add_actor_with_properties (a_info->table,
                                        a_info->row,
                                        -1, 0,
                                        "row-span", 1,
                                        "col-span", 1,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "allocate-hidden", FALSE,
                                        NULL);

  g_free (a_info->name);
  g_object_unref (a_info->account);
  g_slice_free (AccountInfo, a_info);

  return FALSE;
}

static void
on_mc_get_online (MissionControl *mc,
                  GError         *error,
                  gpointer        data)
{
  NbtkTable *table = data;
  GSList *accounts, *a;
  GError *internal_error;

  if (error)
    {
      g_warning ("Unable to go online: %s", error->message);
      g_error_free (error);

      return;
    }

  internal_error = NULL;
  accounts = mission_control_get_online_connections (mc, &internal_error);
  for (a = accounts; a != NULL; a = a->next)
    {
      McAccount *account = a->data;
      AccountInfo *a_info;

      a_info = g_slice_new (AccountInfo);
      a_info->name = g_strdup (mc_account_get_unique_name (account));
      a_info->account = g_object_ref (account);
      a_info->row = NULL;
      a_info->table = table;
      a_info->is_visible = FALSE;

      clutter_threads_add_idle (add_account, a_info);
    }

  g_slist_free (accounts);
}

ClutterActor *
make_status (void)
{
  ClutterActor *table;
  NbtkWidget *header, *label;
  MojitoClient *client;
  MissionControl *mc;

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (table), "MnbStatusPageTable");
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 6);
  clutter_actor_set_reactive (table, TRUE);

  header = nbtk_label_new (_("Your current status"));
  nbtk_widget_set_style_class_name (header, "MnbStatusPageHeader");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (header),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  empty_bin = CLUTTER_ACTOR (nbtk_bin_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (empty_bin), "status-empty-bin");
  nbtk_bin_set_alignment (NBTK_BIN (empty_bin), NBTK_ALIGN_LEFT, NBTK_ALIGN_CENTER);
  nbtk_bin_set_fill (NBTK_BIN (empty_bin), TRUE, FALSE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table), empty_bin,
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 1,
                                        "allocate-hidden", FALSE,
                                        NULL);

  label = nbtk_label_new (_("To update your web status you need to setup "
                            "a Web Services account with a provider that "
                            "supports status messages"));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (label), "status-empty-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (empty_bin),
                               CLUTTER_ACTOR (label));

  /* mojito: web services */
  client = mojito_client_new ();

  /* online notification on the header */
  mojito_client_is_online (client, on_mojito_is_online, header);
  g_signal_connect (client, "online-changed",
                    G_CALLBACK (on_mojito_online_changed),
                    header);

  /* start retrieving the services */
  mojito_client_get_services (client, on_mojito_get_services, table);
  g_object_set_data_full (G_OBJECT (table), "mojito-client",
                          client,
                          (GDestroyNotify) g_object_unref);

  /* mission control: instant messaging */
  mc = mission_control_new (dbus_g_bus_get (DBUS_BUS_SESSION, NULL));

  /* connect everything */
  mission_control_connect_all_with_default_presence (mc,
                                                     on_mc_get_online,
                                                     table);

  g_object_set_data_full (G_OBJECT (table), "mission-control",
                          mc,
                          (GDestroyNotify) g_object_unref);

  return table;
}

static void
resize_status (ClutterActor           *stage,
               const ClutterActorBox  *allocation,
               ClutterAllocationFlags  flags,
               ClutterActor           *table)
{
  clutter_actor_set_width (table, allocation->x2 - allocation->x1);
}

static void
setup_standalone (void)
{
  ClutterActor *stage, *status;

  status = make_status ();

  stage = clutter_stage_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), status);

  g_signal_connect (stage,
                    "allocation-changed", G_CALLBACK (resize_status),
                    status);
  g_signal_connect (stage,
                    "destroy", G_CALLBACK (clutter_main_quit),
                    NULL);

  clutter_actor_set_size (stage, 1024, 600);
  clutter_actor_show (stage);
}

#if 0
static void
setup_embedded (void)
{
  MnbPanelClient *panel;
  ClutterActor *stage, *status;

  panel = mnb_panel_clutter_new (MNB_PANEL_STATUS,
                                 _("status"),
                                 MUTTER_MOBLIN_CSS,
                                 "status-button",
                                 TRUE);

  status = make_status ();
  g_signal_connect (panel,
                    "show-begin", G_CALLBACK (on_status_show_begin),
                    status);
  g_signal_connect (panel,
                    "show-end", G_CALLBACK (on_status_show_end),
                    status);
  g_signal_connect (panel,
                    "hide-end", G_CALLBACK (on_status_hide_end),
                    status);

  stage = mnb_panel_clutter_get_stage (MNB_PANEL_CLUTTER (panel));
  g_signal_connect (stage,
                    "allocation-changed", G_CALLBACK (resize_status),
                    status);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), status);
}
#endif

static gboolean status_standalone = FALSE;

static GOptionEntry status_options[] = {
  {
    "standalone", 's',
    0,
    G_OPTION_ARG_NONE, &status_standalone,
    "Do not embed into mutter-moblin", NULL
  },

  { NULL }
};

int
main (int argc, char *argv[])
{
  GError *error;

  error = NULL;
  clutter_init_with_args (&argc, &argv,
                          "- Mutter-moblin's status page",
                          status_options,
                          GETTEXT_PACKAGE,
                          &error);

  if (error)
    {
      g_critical ("Unable to initialize Clutter: %s", error->message);
      g_clear_error (&error);
      return EXIT_FAILURE;
    }

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             MUTTER_MOBLIN_CSS,
                             &error);
  if (error)
    {
      g_critical ("Unbale to load theme: %s", error->message);
      g_clear_error (&error);
    }

  if (status_standalone)
    setup_standalone ();
  else
    return EXIT_FAILURE;

  clutter_main ();

  return EXIT_SUCCESS;
}

/* vim:set ts=8 expandtab */
