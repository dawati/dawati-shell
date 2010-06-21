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
#include "ggg-manual-dialog.h"

struct _GggManualDialogPrivate {
  GtkWidget *apn;
  GtkWidget *username;
  GtkWidget *password;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GGG_TYPE_MANUAL_DIALOG, GggManualDialogPrivate))
G_DEFINE_TYPE (GggManualDialog, ggg_manual_dialog, GTK_TYPE_DIALOG);

static void
ggg_manual_dialog_class_init (GggManualDialogClass *klass)
{
    g_type_class_add_private (klass, sizeof (GggManualDialogPrivate));
}

#define MAKE_LABEL(s, row)                                              \
  {                                                                     \
    PangoAttrList *attrs;                                               \
    PangoAttribute *attr;                                               \
    label = gtk_label_new (s);                                          \
    attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);                   \
    attr->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;           \
    attr->end_index = PANGO_ATTR_INDEX_TO_TEXT_END;                     \
    attrs = pango_attr_list_new ();                                     \
    pango_attr_list_insert (attrs, attr);                               \
    gtk_label_set_attributes (GTK_LABEL (label), attrs);                \
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);                \
    gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row + 1); \
  }
#define MAKE_ENTRY(var, row)                                            \
  entry = gtk_entry_new ();                                             \
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, row, row + 1); \
  self->priv->var = entry;

static void
ggg_manual_dialog_init (GggManualDialog *self)
{
  GtkWidget *table, *label, *entry;

  self->priv = GET_PRIVATE (self);

  g_object_set (self,
                "title", _("Cellular Data Connection Wizard"),
                "resizable", FALSE,
                "has-separator", FALSE,
                NULL);

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  table = gtk_table_new (4, 2, FALSE);
  g_object_set (table,
                "row-spacing", 6,
                "column-spacing", 6,
                "border-width", 6,
                NULL);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), table);

  label = gtk_label_new (_("Manual Configuration"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 2, 0, 1);

  MAKE_LABEL (_("Plan Name (required):"), 1);
  MAKE_ENTRY (apn, 1);

  MAKE_LABEL (_("Username:"), 2);
  MAKE_ENTRY (username, 2);

  MAKE_LABEL (_("Password:"), 3);
  MAKE_ENTRY (password, 3);

#if 0
  MAKE_LABEL (_("Gateway:"), 4);
  MAKE_ENTRY (gateway, 4);

  MAKE_LABEL (_("Primary DNS:"), 5);
  MAKE_ENTRY (dns1, 5);

  MAKE_LABEL (_("Secondary DNS:"), 6);
  MAKE_ENTRY (dns2, 6);

  MAKE_LABEL (_("Tertiary DNS:"), 7);
  MAKE_ENTRY (dns3, 7);
#endif

  gtk_widget_show_all (table);
}

RestXmlNode *
ggg_manual_dialog_get_plan (GggManualDialog *dialog)
{
  const char *template = "<apn value='%s'>"
    "<username>%s</username>"
    "<password>%s</password>"
    "</apn>";
  char *xml;
  RestXmlParser *parser;
  RestXmlNode *node;

  xml = g_strdup_printf (template,
                         gtk_entry_get_text (GTK_ENTRY (dialog->priv->apn)),
                         gtk_entry_get_text (GTK_ENTRY (dialog->priv->username)),
                         gtk_entry_get_text (GTK_ENTRY (dialog->priv->password)));

  parser = rest_xml_parser_new ();
  node = rest_xml_parser_parse_from_data (parser, xml, strlen (xml));
  g_assert (node);
  g_object_unref (parser);
  g_free (xml);

  return node;
}
