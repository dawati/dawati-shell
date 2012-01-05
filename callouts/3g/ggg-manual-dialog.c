/*
 * Carrick - a connection panel for the Dawati Netbook
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

enum {
  PROP_0,
  PROP_NAME,
  PROP_APN,
  PROP_USERNAME,
  PROP_PASSWORD,
};

struct _GggManualDialogPrivate {
  GtkWidget *label;

  GtkWidget *cancel_button;
  GtkWidget *reconf_button;
  GtkWidget *ok_button;
  GtkWidget *close_button;

  const char *name;

  GtkWidget *apn;
  GtkWidget *username;
  GtkWidget *password;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GGG_TYPE_MANUAL_DIALOG, GggManualDialogPrivate))
G_DEFINE_TYPE (GggManualDialog, ggg_manual_dialog, GTK_TYPE_DIALOG);

static void
ggg_manual_dialog_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  GggManualDialog *self = GGG_MANUAL_DIALOG (object);
  const char *str;

  switch (property_id) {
  case PROP_NAME:
    str = g_value_get_string (value);
    if (str)
      self->priv->name = g_strdup (str);
    break;
  case PROP_APN:
    str = g_value_get_string (value);
    if (str)
      gtk_entry_set_text (GTK_ENTRY (self->priv->apn), g_strdup (str));
    break;
  case PROP_USERNAME:
    str = g_value_get_string (value);
    if (str)
      gtk_entry_set_text (GTK_ENTRY (self->priv->username), g_strdup (str));
    break;
  case PROP_PASSWORD:
    str = g_value_get_string (value);
    if (str)
      gtk_entry_set_text (GTK_ENTRY (self->priv->password), g_strdup (str));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static GObject *
ggg_manual_dialog_constructor (GType                  gtype,
                               guint                  n_properties,
                               GObjectConstructParam *properties)
{
  GObject            *obj;
  GggManualDialog    *dialog;
  GObjectClass       *parent_class;
  gboolean            view_current;
  guint               name_len;
  char               *str;

  parent_class = G_OBJECT_CLASS (ggg_manual_dialog_parent_class);
  obj = parent_class->constructor (gtype, n_properties, properties);
  dialog = GGG_MANUAL_DIALOG (obj);
  name_len = dialog->priv->name ? strlen (dialog->priv->name) : 0;

  /* see if any properties were set */
  view_current = 
    name_len > 0 ||
    gtk_entry_get_text_length (GTK_ENTRY (dialog->priv->apn)) > 0 ||
    gtk_entry_get_text_length (GTK_ENTRY (dialog->priv->username)) > 0 ||
    gtk_entry_get_text_length (GTK_ENTRY (dialog->priv->password)) > 0;

  if (view_current) {
    /* TRANSLATORS: placeholder is one or a few words describing the SIM config,
     * e.g. "Vodafone Internet" */
    if (name_len > 0)
      str = g_strdup_printf (_("Current configuration for %s"),
                             dialog->priv->name);
    else
      str = g_strdup (_("Modify configuration"));
  } else {
    str = g_strdup (_("Manual Configuration"));
  }
  gtk_label_set_text (GTK_LABEL (dialog->priv->label), str);
  g_free (str);

  gtk_widget_set_sensitive (dialog->priv->apn, !view_current);
  gtk_widget_set_sensitive (dialog->priv->username, !view_current);
  gtk_widget_set_sensitive (dialog->priv->password, !view_current);

  gtk_widget_set_visible (dialog->priv->cancel_button, !view_current);
  gtk_widget_set_visible (dialog->priv->reconf_button, view_current);
  gtk_widget_set_visible (dialog->priv->ok_button, !view_current);
  gtk_widget_set_visible (dialog->priv->close_button, view_current);

  return obj;
}

static void
ggg_manual_dialog_class_init (GggManualDialogClass *klass)
{
  GObjectClass *o_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (GggManualDialogPrivate));
  o_class->set_property = ggg_manual_dialog_set_property;
  o_class->constructor = ggg_manual_dialog_constructor;

  pspec = g_param_spec_string ("name", "name", "Connection name",
                               "",
                              G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("apn", "apn", "Accesspoint name",
                               "",
                              G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_APN, pspec);

  pspec = g_param_spec_string ("username", "username", "Username",
                               "",
                              G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_USERNAME, pspec);

  pspec = g_param_spec_string ("password", "password", "Password",
                               "",
                              G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (o_class, PROP_PASSWORD, pspec);
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
  GtkWidget *content, *table, *label, *entry;

  self->priv = GET_PRIVATE (self);

  content = gtk_dialog_get_content_area (GTK_DIALOG (self));

  g_object_set (self,
                "title", _("Cellular Data Connection Wizard"),
                "resizable", FALSE,
                NULL);

  self->priv->cancel_button = gtk_dialog_add_button (GTK_DIALOG (self),
                                                     GTK_STOCK_CANCEL,
                                                     GTK_RESPONSE_CANCEL);
  self->priv->reconf_button = gtk_dialog_add_button (GTK_DIALOG (self),
                                                     _("Configure again"),
                                                     GTK_RESPONSE_REJECT);
  self->priv->ok_button = gtk_dialog_add_button (GTK_DIALOG (self),
                                                 GTK_STOCK_OK,
                                                 GTK_RESPONSE_ACCEPT);
  self->priv->close_button = gtk_dialog_add_button (GTK_DIALOG (self),
                                                    GTK_STOCK_CLOSE,
                                                    GTK_RESPONSE_CLOSE);

  table = gtk_table_new (4, 2, FALSE);
  g_object_set (table,
                "row-spacing", 6,
                "column-spacing", 6,
                "border-width", 6,
                NULL);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (content), table);

  self->priv->label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (self->priv->label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), self->priv->label, 0, 2, 0, 1);

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
