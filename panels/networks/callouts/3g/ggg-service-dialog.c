/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <rest/rest-xml-parser.h>
#include "ggg-service-dialog.h"
#include "ggg-mobile-info.h"

enum {
  PROP_0,
  PROP_SERVICES
};

struct _GggServiceDialogPrivate {
  GtkListStore *store;
  GtkWidget *combo;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GGG_TYPE_SERVICE_DIALOG, GggServiceDialogPrivate))
G_DEFINE_TYPE (GggServiceDialog, ggg_service_dialog, GTK_TYPE_DIALOG);

static void
ggg_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  GggServiceDialog *dialog = GGG_SERVICE_DIALOG (object);

  switch (property_id) {
  case PROP_SERVICES:
    {
      GList *services;
      services = g_value_get_pointer (value);

      for (; services; services = services->next) {
        GggService *service = services->data;

        if (ggg_service_is_roaming (service)) {
          char *name;

          name = g_strdup_printf (_("%s (Roaming)"), ggg_service_get_name (service));

          gtk_list_store_insert_with_values (dialog->priv->store, NULL, 0,
                                             0, service,
                                             1, name,
                                             -1);
          g_free (name);
        } else {
          gtk_list_store_insert_with_values (dialog->priv->store, NULL, 0,
                                             0, service,
                                             1, ggg_service_get_name (service),
                                             -1);
        }
      }
      gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->combo), 0);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
ggg_service_dialog_class_init (GggServiceDialogClass *klass)
{
  GObjectClass *o_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (GggServiceDialogPrivate));

  o_class->set_property = ggg_service_set_property;

  pspec = g_param_spec_pointer ("services", "services", "services",
                                G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_SERVICES, pspec);
}

static void
ggg_service_dialog_init (GggServiceDialog *self)
{
  GggServiceDialogPrivate *priv;
  GtkWidget *table, *image, *label;
  GtkCellRenderer *cell;

  priv = self->priv = GET_PRIVATE (self);

  gtk_window_set_title (GTK_WINDOW (self), _("Cellular Data Connection Wizard"));

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  table = gtk_table_new (2, 5, FALSE);
  g_object_set (table,
                "row-spacing", 6,
                "column-spacing", 6,
                "border-width", 6,
                NULL);
  gtk_widget_show (table);

  image = gtk_image_new_from_stock (GTK_STOCK_NETWORK, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_table_attach_defaults (GTK_TABLE (table), image, 0, 1, 0, 2);

  label = gtk_label_new (_("Select 3G Service"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, 0, 1);

  priv->store = gtk_list_store_new (2, REST_TYPE_XML_NODE, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->store), 1, GTK_SORT_ASCENDING);

  priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->store));
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo), cell,
                                  "text", 1,
                                  NULL);
  gtk_widget_show (priv->combo);
  gtk_table_attach_defaults (GTK_TABLE (table), priv->combo, 1, 3, 1, 2);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), table);
}

GggService *
ggg_service_dialog_get_selected (GggServiceDialog *dialog)
{
  GtkComboBox *combo;
  GtkTreeIter iter;

  combo = GTK_COMBO_BOX (dialog->priv->combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    GggService *service;
    gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, 0, &service, -1);
    return service;
  } else {
    return NULL;
  }
}
