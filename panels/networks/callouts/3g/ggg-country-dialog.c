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
#include "ggg-country-dialog.h"
#include "ggg-mobile-info.h"
#include "ggg-iso.h"

struct _GggCountryDialogPrivate {
  GtkWidget *combo;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GGG_TYPE_COUNTRY_DIALOG, GggCountryDialogPrivate))
G_DEFINE_TYPE (GggCountryDialog, ggg_country_dialog, GTK_TYPE_DIALOG);

static void
ggg_country_dialog_class_init (GggCountryDialogClass *klass)
{
  g_type_class_add_private (klass, sizeof (GggCountryDialogPrivate));
}

static void
populate_store (GtkListStore *store)
{
  RestXmlNode *root, *node;

  root = ggg_mobile_info_get_root ();

  node = rest_xml_node_find (root, "country");
  for (; node; node = node->next) {
    const char *country;

    country = ggg_iso_country_name_for_code (rest_xml_node_get_attr (node, "code"));

    gtk_list_store_insert_with_values (store, NULL, 0,
                                       0, node,
                                       1, g_dgettext ("iso_3166", country),
                                       -1);
  }
}

static void
ggg_country_dialog_init (GggCountryDialog *self)
{
  GggCountryDialogPrivate *priv;
  GtkWidget *table, *image, *label;
  GtkListStore *store;
  GtkCellRenderer *cell;

  priv = self->priv = GET_PRIVATE (self);

  gtk_window_set_title (GTK_WINDOW (self), _("Cellular Data Connection Wizard"));

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          "Manual Configuration", GTK_RESPONSE_REJECT,
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

  label = gtk_label_new (_("Select Country"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, 0, 1);

  store = gtk_list_store_new (2, REST_TYPE_XML_NODE, G_TYPE_STRING);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 1, GTK_SORT_ASCENDING);
  populate_store (store);

  priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo), cell,
                                  "text", 1,
                                  NULL);
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo), 0);
  gtk_widget_show (priv->combo);
  gtk_table_attach_defaults (GTK_TABLE (table), priv->combo, 1, 3, 1, 2);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), table);
}

RestXmlNode *
ggg_country_dialog_get_selected (GggCountryDialog *dialog)
{
  GtkComboBox *combo;
  GtkTreeIter iter;

  combo = GTK_COMBO_BOX (dialog->priv->combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    GtkTreeModel *model;
    RestXmlNode *node;

    model = gtk_combo_box_get_model (combo);
    gtk_tree_model_get (model, &iter, 0, &node, -1);
    return node;
  } else {
    return NULL;
  }
}
