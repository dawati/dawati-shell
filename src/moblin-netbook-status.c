/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "moblin-netbook.h"
#include "moblin-netbook-status.h"
#include "mnb-drop-down.h"
#include "mnb-status-row.h"
#include <nbtk/nbtk.h>
#include <gtk/gtk.h>
#include <string.h>

#include <mojito-client/mojito-client.h>

#define ICON_SIZE       48
#define PADDING         8
#define BORDER_WIDTH    4

typedef struct _GetCapabilitiesClosure
{
  MojitoClientService *service;
  gchar *service_name;
  NbtkTable *table;
  guint row_number;
} GetCapabilitiesClosure;

static void
on_mojito_get_capabilities (MojitoClientService *service,
                            guint32              caps,
                            const GError        *error,
                            gpointer             data)
{
  GetCapabilitiesClosure *closure = data;

  if (error)
    {
      g_warning ("Unable to get capabilities of service '%s': %s",
                 closure->service_name,
                 error->message);
      goto out;
    }

  g_debug ("%s: GetCapabilities %s "
           "[update-status: %s, get-persona-icon: %s][%d]",
           G_STRLOC,
           closure->service_name,
           caps & MOJITO_CLIENT_SERVICE_CAN_UPDATE_STATUS    ? "y" : "n",
           caps & MOJITO_CLIENT_SERVICE_CAN_GET_PERSONA_ICON ? "y" : "n",
           closure->row_number);

  if ((caps & MOJITO_CLIENT_SERVICE_CAN_UPDATE_STATUS) &&
      (caps & MOJITO_CLIENT_SERVICE_CAN_GET_PERSONA_ICON))
    {
      NbtkWidget *row = g_object_new (MNB_TYPE_STATUS_ROW,
                                      "service-name", closure->service_name,
                                      NULL);
      g_assert (row != NULL);

      g_debug ("%s: Adding row %d for service %s",
               G_STRLOC,
               closure->row_number,
               closure->service_name);

      nbtk_table_add_widget (closure->table, row, closure->row_number, 0);
    }

out:
  g_object_unref (closure->table);
  g_object_unref (closure->service);
  g_free (closure->service_name);
  g_slice_free (GetCapabilitiesClosure, closure);
}

static void
on_mojito_get_services (MojitoClient *client,
                        const GList  *services,
                        gpointer      data)
{
  NbtkTable *table = data;
  const GList *l;
  gint i;

  for (l = services, i = 1; l != NULL; l = l->next, i++)
    {
      const gchar *service_name = l->data;
      MojitoClientService *service;
      GetCapabilitiesClosure *closure;

      /* filter out the dummy service */
      if (strcmp (service_name, "dummy") == 0)
        {
          i -= 1;
          continue;
        }

      service = mojito_client_get_service (client, service_name);
      if (G_UNLIKELY (service == NULL))
        {
          i -= 1;
          continue;
        }

      g_debug ("%s: GetServices [%s][%i]", G_STRLOC, service_name, i);

      closure = g_slice_new0 (GetCapabilitiesClosure);
      closure->service = g_object_ref (service);
      closure->service_name = g_strdup (service_name);
      closure->table = g_object_ref (table);
      closure->row_number = i;

      mojito_client_service_get_capabilities (service,
                                              on_mojito_get_capabilities,
                                              closure);
    }
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
      if (MNB_IS_STATUS_ROW (l->data))
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
  MojitoClient  *client;

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (table), "MnbStatusPageTable");
  clutter_actor_set_width (CLUTTER_ACTOR (table), width);
  clutter_actor_set_reactive (table, TRUE);

  header = nbtk_label_new ("Your current status");
  nbtk_widget_set_style_class_name (header, "MnbStatusPageHeader");
  nbtk_table_add_widget_full (NBTK_TABLE (table), header,
                              0, 0,
                              1, 1,
                              0,
                              0.0, 0.5);

  client = mojito_client_new ();
  mojito_client_get_services (client, on_mojito_get_services, table);
  g_object_set_data_full (G_OBJECT (table), "mojito-client",
                          client,
                          (GDestroyNotify) g_object_unref);

  drop_down = mnb_drop_down_new ();
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), table);
  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (on_drop_down_show_completed),
                    table);

  return CLUTTER_ACTOR (drop_down);
}

/* vim:set ts=8 expandtab */
