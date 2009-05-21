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

#include "moblin-netbook.h"
#include "moblin-netbook-status.h"
#include "mnb-drop-down.h"
#include "mnb-status-row.h"

#include <string.h>

#include <glib/gi18n.h>

#include <nbtk/nbtk.h>
#include <gtk/gtk.h>

#include <mojito-client/mojito-client.h>

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
          s_info->row = g_object_new (MNB_TYPE_STATUS_ROW,
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

static void
on_drop_down_show_completed (MnbDropDown *drop_down,
                             NbtkTable   *table)
{
  ClutterContainer *container = CLUTTER_CONTAINER (table);
  GList *children, *l;

  children = clutter_container_get_children (container);
  for (l = children; l != NULL; l = l->next)
    {
      if (MNB_IS_STATUS_ROW (l->data) && CLUTTER_ACTOR_IS_VISIBLE (l->data))
        mnb_status_row_force_update (l->data);
    }

  g_list_free (children);
}

ClutterActor *
make_status (MutterPlugin *plugin, gint width)
{
  ClutterActor  *table;
  NbtkWidget    *drop_down;
  NbtkWidget    *header;
  NbtkWidget    *label;
  MojitoClient  *client;

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (table), "MnbStatusPageTable");
  clutter_actor_set_width (CLUTTER_ACTOR (table), width);
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

  drop_down = mnb_drop_down_new (plugin);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), table);
  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (on_drop_down_show_completed),
                    table);

  return CLUTTER_ACTOR (drop_down);
}

/* vim:set ts=8 expandtab */
